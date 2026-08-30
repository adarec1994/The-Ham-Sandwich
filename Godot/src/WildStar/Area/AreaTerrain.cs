using System;
using System.Globalization;

namespace WildStar.Area;

public readonly struct AreaTileCoord
{
    public AreaTileCoord(int x, int z, bool low)
    {
        X = x;
        Z = z;
        Low = low;
    }

    public int X { get; }

    public int Z { get; }

    public bool Low { get; }

    public float TileSize => Low ? AreaTerrain.LowTileSize : AreaTerrain.TileSize;

    public float OriginX => Low ? (X * 8 - AreaTerrain.TileOriginOffset) * AreaTerrain.TileSize
                                : (X - AreaTerrain.TileOriginOffset) * AreaTerrain.TileSize;

    public float OriginZ => Low ? (Z * 8 - AreaTerrain.TileOriginOffset) * AreaTerrain.TileSize
                                : (Z - AreaTerrain.TileOriginOffset) * AreaTerrain.TileSize;

    public static bool TryParse(string fileName, out string mapName, out AreaTileCoord coord)
    {
        mapName = string.Empty;
        coord = default;
        int slash = fileName.LastIndexOfAny(new[] { '/', '\\' });
        string leaf = slash >= 0 ? fileName[(slash + 1)..] : fileName;
        if (!leaf.EndsWith(".area", StringComparison.OrdinalIgnoreCase))
        {
            return false;
        }

        string stem = leaf[..^5];
        int dot = stem.LastIndexOf('.');
        if (dot <= 0 || stem.Length - dot - 1 != 4)
        {
            return false;
        }

        string hex = stem[(dot + 1)..];
        if (!int.TryParse(hex.AsSpan(0, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out int zz) ||
            !int.TryParse(hex.AsSpan(2, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out int xx))
        {
            return false;
        }

        string name = stem[..dot];
        bool low = name.EndsWith("_Low", StringComparison.OrdinalIgnoreCase);
        mapName = low ? name[..^4] : name;
        coord = new AreaTileCoord(xx, zz, low);
        return true;
    }
}

public static class AreaTerrain
{
    public const float HeightScale = 0.12500381f;
    public const float HeightOffset = 2048.0f;
    public const float LowHeightDrop = 8.0f;
    public const float CellSize = 2.0f;
    public const float ChunkSize = 32.0f;
    public const float TileSize = 512.0f;
    public const float LowCellSize = 8.0f;
    public const float LowChunkSize = 256.0f;
    public const float LowTileSize = 4096.0f;
    public const int TileOriginOffset = 64;
    public const int GlobalChunkOffset = 1024;
    public const int CellsPerChunk = 16;
    public const int VerticesPerSide = 17;
    public const int CornerVertexCount = 289;
    public const int CentreVertexCount = 256;
    public const int VertexCount = 545;
    public const int LowCellsPerChunk = 32;
    public const int LowVerticesPerSide = 33;
    public const float MapTextureTexels = 68.0f;
    public const float MapTextureUsedTexels = 64.0f;

    public static float DecodeHeight(ushort raw) => (raw & 0x7FFF) * HeightScale - HeightOffset;

    public static int CornerIndex(int i, int j) => i + VerticesPerSide * j;

    public static int CentreIndex(int x, int y) => CornerVertexCount + x + CellsPerChunk * y;

    public static float[] CellCentreHeights(AreaChunk chunk)
    {
        var centres = new float[18 * 18];
        for (int r = 0; r < 18; r++)
        {
            for (int c = 0; c < 18; c++)
            {
                float h00 = chunk.Height(c - 1, r - 1);
                float h10 = chunk.Height(c, r - 1);
                float h01 = chunk.Height(c - 1, r);
                float h11 = chunk.Height(c, r);
                float wa = 1.0f / ((h00 - h11) * (h00 - h11) + 1.0f);
                float wb = 1.0f / ((h10 - h01) * (h10 - h01) + 1.0f);
                centres[r * 18 + c] = ((h10 + h01) * wb + (h00 + h11) * wa) * (0.5f / (wb + wa));
            }
        }

        return centres;
    }

    public static void BuildChunkGeometry(AreaChunk chunk, out float[] positions, out float[] normals,
                                          out bool[] holes)
    {
        float[] cc = CellCentreHeights(chunk);
        positions = new float[VertexCount * 3];
        normals = new float[VertexCount * 3];
        holes = new bool[CellsPerChunk * CellsPerChunk];

        for (int j = 0; j < VerticesPerSide; j++)
        {
            for (int i = 0; i < VerticesPerSide; i++)
            {
                float c00 = cc[j * 18 + i];
                float c10 = cc[j * 18 + i + 1];
                float c01 = cc[(j + 1) * 18 + i];
                float c11 = cc[(j + 1) * 18 + i + 1];
                float nx = ((c11 + c10) - (c01 + c00)) * -0.5f;
                float nz = ((c11 + c01) - (c10 + c00)) * -0.5f;
                float ny = CellSize;
                float inv = 1.0f / MathF.Sqrt(nx * nx + ny * ny + nz * nz);
                int v = CornerIndex(i, j);
                positions[3 * v] = i * CellSize;
                positions[3 * v + 1] = chunk.Height(i, j);
                positions[3 * v + 2] = j * CellSize;
                normals[3 * v] = nx * inv;
                normals[3 * v + 1] = ny * inv;
                normals[3 * v + 2] = nz * inv;
            }
        }

        for (int y = 0; y < CellsPerChunk; y++)
        {
            for (int x = 0; x < CellsPerChunk; x++)
            {
                int v00 = CornerIndex(x, y);
                int v10 = CornerIndex(x + 1, y);
                int v01 = CornerIndex(x, y + 1);
                int v11 = CornerIndex(x + 1, y + 1);
                float h00 = positions[3 * v00 + 1];
                float h10 = positions[3 * v10 + 1];
                float h01 = positions[3 * v01 + 1];
                float h11 = positions[3 * v11 + 1];
                float wb = 1.0f / ((h10 - h01) * (h10 - h01) + 1.0f);
                float wa = 1.0f / ((h00 - h11) * (h00 - h11) + 1.0f);
                float k = 0.5f / (wb + wa);
                float kb = k * wb;
                float ka = k * wa;
                int c = CentreIndex(x, y);
                positions[3 * c] = x * CellSize + CellSize * 0.5f;
                positions[3 * c + 1] = kb * (h10 + h01) + ka * (h00 + h11);
                positions[3 * c + 2] = y * CellSize + CellSize * 0.5f;
                float nx = kb * (normals[3 * v10] + normals[3 * v01]) + ka * (normals[3 * v00] + normals[3 * v11]);
                float ny = kb * (normals[3 * v10 + 1] + normals[3 * v01 + 1]) + ka * (normals[3 * v00 + 1] + normals[3 * v11 + 1]);
                float nz = kb * (normals[3 * v10 + 2] + normals[3 * v01 + 2]) + ka * (normals[3 * v00 + 2] + normals[3 * v11 + 2]);
                float inv = 1.0f / MathF.Sqrt(nx * nx + ny * ny + nz * nz);
                normals[3 * c] = nx * inv;
                normals[3 * c + 1] = ny * inv;
                normals[3 * c + 2] = nz * inv;
                holes[y * CellsPerChunk + x] = chunk.IsHole(x, y);
            }
        }
    }

    public static int[] BuildChunkIndices(bool[] holes)
    {
        int cells = 0;
        foreach (bool hole in holes)
        {
            if (!hole)
            {
                cells++;
            }
        }

        var indices = new int[cells * 12];
        int n = 0;
        for (int y = 0; y < CellsPerChunk; y++)
        {
            for (int x = 0; x < CellsPerChunk; x++)
            {
                if (holes[y * CellsPerChunk + x])
                {
                    continue;
                }

                int v00 = CornerIndex(x, y);
                int v10 = CornerIndex(x + 1, y);
                int v01 = CornerIndex(x, y + 1);
                int v11 = CornerIndex(x + 1, y + 1);
                int c = CentreIndex(x, y);
                indices[n++] = v00; indices[n++] = v10; indices[n++] = c;
                indices[n++] = v10; indices[n++] = v11; indices[n++] = c;
                indices[n++] = v11; indices[n++] = v01; indices[n++] = c;
                indices[n++] = v01; indices[n++] = v00; indices[n++] = c;
            }
        }

        return indices;
    }

    public static float[] LowHeights(AreaChunk chunk)
    {
        var heights = new float[LowVerticesPerSide * LowVerticesPerSide];
        for (int y = 0; y < LowVerticesPerSide; y++)
        {
            for (int x = 0; x < LowVerticesPerSide; x++)
            {
                heights[y * LowVerticesPerSide + x] = DecodeHeight(chunk.LowHeightRaw(x, y)) - LowHeightDrop;
            }
        }

        return heights;
    }

    public static float ChunkOriginX(AreaTileCoord tile, AreaChunk chunk) =>
        tile.OriginX + chunk.X * (tile.Low ? LowChunkSize : ChunkSize);

    public static float ChunkOriginZ(AreaTileCoord tile, AreaChunk chunk) =>
        tile.OriginZ + chunk.Y * (tile.Low ? LowChunkSize : ChunkSize);
}
