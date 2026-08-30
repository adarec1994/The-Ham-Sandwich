using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace WildStar.Area;

public sealed class AreaChunkCompute
{
    public AreaChunk Chunk = null!;

    public float[] Positions = Array.Empty<float>();

    public float[] Normals = Array.Empty<float>();

    public float[] Uvs = Array.Empty<float>();

    public int[] Indices = Array.Empty<int>();

    public float OffsetX;

    public float OffsetZ;

    public uint[] WorldLayerIds = new uint[4];

    public byte[]? BlendMap;

    public byte[]? BlendMapDxt;

    public byte[]? ColourMap;

    public byte[]? ColourMapDxt;

    public bool HasColourMap;

    public const int MapSize = 65;
}

public sealed class AreaTileCompute
{
    public string MapName = string.Empty;

    public AreaTileCoord Tile;

    public string Name = string.Empty;

    public AreaFile Area = null!;

    public List<AreaChunkCompute> Chunks = new();

    public uint LayerUnion;

    public Dictionary<uint, int> SkyVotes = new();

    public int[] ChunkSkyIds = new int[256];

    public int[] ChunkSkyQuadrantIds = new int[256 * AreaSky.ValuesPerChunk];

    public byte[] ChunkSkyQuadrantWeights = new byte[256 * AreaSky.ValuesPerChunk];

    public static AreaTileCompute Compute(AreaFile area, string mapName, AreaTileCoord tile,
                                          string name, bool parallelChunks)
    {
        var result = new AreaTileCompute { MapName = mapName, Tile = tile, Name = name, Area = area };

        var present = new List<AreaChunk>();
        foreach (AreaChunk chunk in area.PresentChunks())
        {
            present.Add(chunk);
            result.LayerUnion |= chunk.LayerFlags;
            if (chunk.HasSkyBlend && chunk.Index >= 0 && chunk.Index < 256)
            {
                Span<uint> ids = stackalloc uint[AreaSky.ValuesPerChunk];
                Span<byte> weights = stackalloc byte[AreaSky.ValuesPerChunk];
                chunk.SkyQuadrantIds(ids);
                chunk.SkyQuadrantWeights(weights);
                int baseIndex = chunk.Index * AreaSky.ValuesPerChunk;
                for (int i = 0; i < AreaSky.ValuesPerChunk; i++)
                {
                    result.ChunkSkyQuadrantIds[baseIndex + i] = (int)ids[i];
                    result.ChunkSkyQuadrantWeights[baseIndex + i] = weights[i];
                }

                uint skyId = AreaSky.Dominant(ids, weights);
                if (skyId != 0)
                {
                    result.SkyVotes[skyId] = result.SkyVotes.TryGetValue(skyId, out int n) ? n + 1 : 1;
                    result.ChunkSkyIds[chunk.Index] = (int)skyId;
                }
            }
        }

        var computed = new AreaChunkCompute?[present.Count];
        if (parallelChunks && present.Count > 8)
        {
            Parallel.For(0, present.Count, i => computed[i] = ComputeChunk(present[i], tile));
        }
        else
        {
            for (int i = 0; i < present.Count; i++)
            {
                computed[i] = ComputeChunk(present[i], tile);
            }
        }

        foreach (AreaChunkCompute? chunk in computed)
        {
            if (chunk is not null)
            {
                result.Chunks.Add(chunk);
            }
        }

        return result;
    }

    private static AreaChunkCompute? ComputeChunk(AreaChunk chunk, AreaTileCoord tile) =>
        tile.Low ? ComputeLowChunk(chunk) : ComputeDetailChunk(chunk);

    private static AreaChunkCompute? ComputeDetailChunk(AreaChunk chunk)
    {
        if (!chunk.HasHeights)
        {
            return null;
        }

        AreaTerrain.BuildChunkGeometry(chunk, out float[] positions, out float[] normals, out bool[] holes);
        int[] indices = AreaTerrain.BuildChunkIndices(holes);
        if (indices.Length == 0)
        {
            return null;
        }

        var uvs = new float[AreaTerrain.VertexCount * 2];
        for (int v = 0; v < AreaTerrain.VertexCount; v++)
        {
            uvs[2 * v] = positions[3 * v] / AreaTerrain.ChunkSize;
            uvs[2 * v + 1] = positions[3 * v + 2] / AreaTerrain.ChunkSize;
        }

        var result = new AreaChunkCompute
        {
            Chunk = chunk,
            Positions = positions,
            Normals = normals,
            Uvs = uvs,
            Indices = indices,
            OffsetX = chunk.X * AreaTerrain.ChunkSize,
            OffsetZ = chunk.Y * AreaTerrain.ChunkSize,
        };
        ParseSplatMaps(chunk, result);
        return result;
    }

    private static void ParseSplatMaps(AreaChunk chunk, AreaChunkCompute result)
    {
        if (chunk.Has(AreaLayerTable.TextureIds))
        {
            uint[] ids = chunk.TextureIds();
            for (int j = 0; j < 4 && j < ids.Length; j++)
            {
                result.WorldLayerIds[j] = ids[j];
            }
        }

        const int texels = AreaChunkCompute.MapSize * AreaChunkCompute.MapSize;
        if (chunk.Has(AreaLayerTable.BlendSource))
        {
            byte[] raw = chunk.Layer(AreaLayerTable.BlendSource);
            if (raw.Length >= texels * 2)
            {
                var rgba = new byte[texels * 4];
                for (int i = 0; i < texels; i++)
                {
                    int val = raw[2 * i] | (raw[2 * i + 1] << 8);
                    rgba[i * 4 + 0] = (byte)(((val >> 0) & 0xF) * 255 / 15);
                    rgba[i * 4 + 1] = (byte)(((val >> 4) & 0xF) * 255 / 15);
                    rgba[i * 4 + 2] = (byte)(((val >> 8) & 0xF) * 255 / 15);
                    rgba[i * 4 + 3] = (byte)(((val >> 12) & 0xF) * 255 / 15);
                }

                result.BlendMap = rgba;
            }
        }

        if (chunk.Has(AreaLayerTable.ColourSource))
        {
            byte[] raw = chunk.Layer(AreaLayerTable.ColourSource);
            if (raw.Length >= texels * 2)
            {
                var rgba = new byte[texels * 4];
                for (int i = 0; i < texels; i++)
                {
                    int val = raw[2 * i] | (raw[2 * i + 1] << 8);
                    int r5 = (val >> 0) & 0x1F;
                    int g6 = (val >> 5) & 0x3F;
                    int b5 = (val >> 11) & 0x1F;
                    rgba[i * 4 + 0] = (byte)(r5 * 255 / 31);
                    rgba[i * 4 + 1] = (byte)(g6 * 255 / 63);
                    rgba[i * 4 + 2] = (byte)(b5 * 255 / 31);
                    rgba[i * 4 + 3] = 255;
                }

                result.ColourMap = rgba;
            }
        }

        if (chunk.Has(AreaLayerTable.ColourMap))
        {
            byte[] dxt = chunk.Layer(AreaLayerTable.ColourMap);
            if (dxt.Length >= 4624)
            {
                result.ColourMapDxt = dxt.Length == 4624 ? dxt : dxt.AsSpan(0, 4624).ToArray();
            }
        }

        if (chunk.Has(AreaLayerTable.BlendMap))
        {
            byte[] dxt = chunk.Layer(AreaLayerTable.BlendMap);
            if (dxt.Length >= 2312)
            {
                result.BlendMapDxt = dxt.Length == 2312 ? dxt : dxt.AsSpan(0, 2312).ToArray();
            }
        }

        result.HasColourMap = chunk.Has(AreaLayerTable.ColourSource) || chunk.Has(AreaLayerTable.ColourMap);
    }

    private static AreaChunkCompute? ComputeLowChunk(AreaChunk chunk)
    {
        if (!chunk.HasLowHeights)
        {
            return null;
        }

        float[] heights = AreaTerrain.LowHeights(chunk);
        const int side = AreaTerrain.LowVerticesPerSide;
        var positions = new float[side * side * 3];
        var normals = new float[side * side * 3];
        var uvs = new float[side * side * 2];

        for (int y = 0; y < side; y++)
        {
            for (int x = 0; x < side; x++)
            {
                int v = y * side + x;
                positions[3 * v] = x * AreaTerrain.LowCellSize;
                positions[3 * v + 1] = heights[v];
                positions[3 * v + 2] = y * AreaTerrain.LowCellSize;
                uvs[2 * v] = x / (float)AreaTerrain.LowCellsPerChunk;
                uvs[2 * v + 1] = y / (float)AreaTerrain.LowCellsPerChunk;

                float hl = heights[y * side + Math.Max(0, x - 1)];
                float hr = heights[y * side + Math.Min(side - 1, x + 1)];
                float hd = heights[Math.Max(0, y - 1) * side + x];
                float hu = heights[Math.Min(side - 1, y + 1) * side + x];
                float nx = hl - hr;
                float ny = 2.0f * AreaTerrain.LowCellSize;
                float nz = hd - hu;
                float length = MathF.Sqrt(nx * nx + ny * ny + nz * nz);
                normals[3 * v] = nx / length;
                normals[3 * v + 1] = ny / length;
                normals[3 * v + 2] = nz / length;
            }
        }

        var indices = new int[AreaTerrain.LowCellsPerChunk * AreaTerrain.LowCellsPerChunk * 6];
        int n = 0;
        for (int y = 0; y < AreaTerrain.LowCellsPerChunk; y++)
        {
            for (int x = 0; x < AreaTerrain.LowCellsPerChunk; x++)
            {
                int v00 = y * side + x;
                int v10 = v00 + 1;
                int v01 = v00 + side;
                int v11 = v01 + 1;
                indices[n++] = v00;
                indices[n++] = v10;
                indices[n++] = v01;
                indices[n++] = v10;
                indices[n++] = v11;
                indices[n++] = v01;
            }
        }

        return new AreaChunkCompute
        {
            Chunk = chunk,
            Positions = positions,
            Normals = normals,
            Uvs = uvs,
            Indices = indices,
            OffsetX = chunk.X * AreaTerrain.LowChunkSize,
            OffsetZ = chunk.Y * AreaTerrain.LowChunkSize,
        };
    }
}
