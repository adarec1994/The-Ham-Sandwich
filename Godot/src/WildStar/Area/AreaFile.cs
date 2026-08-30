using System;
using System.Buffers.Binary;
using System.Collections.Generic;

namespace WildStar.Area;

public readonly struct AreaBlock
{
    public AreaBlock(uint tag, byte[] data)
    {
        Tag = tag;
        Data = data;
    }

    public uint Tag { get; }

    public byte[] Data { get; }

    public string TagName => AreaFile.TagName(Tag);
}

public sealed class AreaChunk
{
    public const int ChunksPerSide = 16;
    public const int ChunkCount = 256;

    public const uint TagProps = 0x50524F50;
    public const uint TagWater = 0x57417447;
    public const uint TagCurtainDecal = 0x63757244;
    public const uint TagWbsP = 0x77627350;

    public AreaChunk(int index, uint layerFlags, byte[]?[] layers, AreaBlock[] blocks)
    {
        Index = index;
        LayerFlags = layerFlags;
        Layers = layers;
        Blocks = blocks;
    }

    public int Index { get; }

    public int X => Index & 15;

    public int Y => Index >> 4;

    public uint LayerFlags { get; }

    public byte[]?[] Layers { get; }

    public AreaBlock[] Blocks { get; }

    public bool Has(int layer) => Layers[layer] is not null;

    public byte[] Layer(int layer) =>
        Layers[layer] ?? throw new InvalidOperationException("layer " + layer + " absent");

    public bool HasHeights => Has(AreaLayerTable.Heights);

    public bool HasLowHeights => Has(AreaLayerTable.LowHeights);

    public ushort HeightRaw(int x, int y)
    {
        AreaLayerInfo info = AreaLayerTable.Entries[AreaLayerTable.Heights];
        return BinaryPrimitives.ReadUInt16LittleEndian(Layer(AreaLayerTable.Heights).AsSpan(info.Offset(x, y)));
    }

    public float Height(int x, int y) => AreaTerrain.DecodeHeight(HeightRaw(x, y));

    public bool IsHole(int x, int y) => (HeightRaw(x, y) & 0x8000) != 0;

    public ushort LowHeightRaw(int x, int y)
    {
        AreaLayerInfo info = AreaLayerTable.Entries[AreaLayerTable.LowHeights];
        return BinaryPrimitives.ReadUInt16LittleEndian(Layer(AreaLayerTable.LowHeights).AsSpan(info.Offset(x, y)));
    }

    public uint[] TextureIds(int layer = AreaLayerTable.TextureIds)
    {
        var ids = new uint[4];
        if (!Has(layer))
        {
            return ids;
        }

        byte[] data = Layer(layer);
        for (int i = 0; i < 4; i++)
        {
            ids[i] = BinaryPrimitives.ReadUInt32LittleEndian(data.AsSpan(4 * i));
        }

        return ids;
    }

    public uint[] ZoneIds()
    {
        if (Has(AreaLayerTable.ZoneIds))
        {
            return TextureIds(AreaLayerTable.ZoneIds);
        }

        if (Has(AreaLayerTable.ZoneId))
        {
            return new[] { BinaryPrimitives.ReadUInt32LittleEndian(Layer(AreaLayerTable.ZoneId)), 0u, 0u, 0u };
        }

        return new uint[4];
    }

    public byte CellFlags(int x, int y)
    {
        AreaLayerInfo info = AreaLayerTable.Entries[AreaLayerTable.CellFlags];
        return Layer(AreaLayerTable.CellFlags)[info.Offset(x, y)];
    }

    public int ZoneSelector(int x, int y) => (CellFlags(x, y) >> 3) & 3;

    public uint ZoneIdAt(int x, int y)
    {
        if (Has(AreaLayerTable.ZoneIds))
        {
            return Has(AreaLayerTable.CellFlags)
                ? TextureIds(AreaLayerTable.ZoneIds)[ZoneSelector(x, y)]
                : TextureIds(AreaLayerTable.ZoneIds)[0];
        }

        return Has(AreaLayerTable.ZoneId) ? BinaryPrimitives.ReadUInt32LittleEndian(Layer(AreaLayerTable.ZoneId)) : 0u;
    }

    public bool TryHeightBounds(out float min, out float max)
    {
        min = 0.0f;
        max = 0.0f;
        if (!Has(AreaLayerTable.HeightBounds))
        {
            return false;
        }

        uint packed = BinaryPrimitives.ReadUInt32LittleEndian(Layer(AreaLayerTable.HeightBounds));
        min = AreaTerrain.DecodeHeight((ushort)(packed & 0xFFFF));
        max = AreaTerrain.DecodeHeight((ushort)(packed >> 16));
        return true;
    }

    public uint DominantZoneId()
    {
        uint[] ids = ZoneIds();
        if (!Has(AreaLayerTable.ZoneIds) || !Has(AreaLayerTable.CellFlags))
        {
            return ids[0];
        }

        var counts = new int[4];
        AreaLayerInfo info = AreaLayerTable.Entries[AreaLayerTable.CellFlags];
        for (int y = info.Y0; y < info.Y1; y++)
        {
            for (int x = info.X0; x < info.X1; x++)
            {
                counts[ZoneSelector(x, y)]++;
            }
        }

        int best = 0;
        for (int i = 1; i < 4; i++)
        {
            if (ids[i] != 0 && counts[i] > counts[best])
            {
                best = i;
            }
        }

        return ids[best] != 0 ? ids[best] : ids[0];
    }

    public bool HasSkyBlend => Has(AreaLayerTable.SkyIds);

    public void SkyQuadrantIds(Span<uint> ids)
    {
        if (!Has(AreaLayerTable.SkyIds))
        {
            ids.Slice(0, AreaSky.ValuesPerChunk).Clear();
            return;
        }

        AreaLayerInfo info = AreaLayerTable.Entries[AreaLayerTable.SkyIds];
        byte[] data = Layer(AreaLayerTable.SkyIds);
        for (int qy = 0; qy < 2; qy++)
        {
            for (int qx = 0; qx < 2; qx++)
            {
                int offset = info.Offset(qx, qy);
                for (int slot = 0; slot < AreaSky.Slots; slot++)
                {
                    ids[AreaSky.Index(qx, qy, slot)] = BinaryPrimitives.ReadUInt32LittleEndian(data.AsSpan(offset + 4 * slot));
                }
            }
        }
    }

    public void SkyQuadrantWeights(Span<byte> weights)
    {
        if (!Has(AreaLayerTable.SkyWeights))
        {
            weights.Slice(0, AreaSky.ValuesPerChunk).Clear();
            return;
        }

        AreaLayerInfo info = AreaLayerTable.Entries[AreaLayerTable.SkyWeights];
        byte[] data = Layer(AreaLayerTable.SkyWeights);
        for (int qy = 0; qy < 2; qy++)
        {
            for (int qx = 0; qx < 2; qx++)
            {
                int offset = info.Offset(qx, qy);
                for (int slot = 0; slot < AreaSky.Slots; slot++)
                {
                    weights[AreaSky.Index(qx, qy, slot)] = data[offset + slot];
                }
            }
        }
    }

    public int SampleSky(float fx, float fy, Span<uint> outIds, Span<float> outWeights)
    {
        Span<uint> ids = stackalloc uint[AreaSky.ValuesPerChunk];
        Span<byte> weights = stackalloc byte[AreaSky.ValuesPerChunk];
        SkyQuadrantIds(ids);
        SkyQuadrantWeights(weights);
        return AreaSky.Sample(ids, weights, fx, fy, outIds, outWeights);
    }

    public uint DominantSkyId()
    {
        Span<uint> ids = stackalloc uint[AreaSky.ValuesPerChunk];
        Span<byte> weights = stackalloc byte[AreaSky.ValuesPerChunk];
        SkyQuadrantIds(ids);
        SkyQuadrantWeights(weights);
        return AreaSky.Dominant(ids, weights);
    }
}

public sealed class AreaFile
{
    public const uint MagicCompiled = 0x61726561;
    public const uint MagicRaw = 0x41524541;
    public const uint TagChunks = 0x43484E4B;
    public const uint TagProps = 0x50524F70;
    public const uint TagCurtains = 0x43555254;
    public const uint TagDhmo = 0x44484D4F;

    public const int PropRecordSize = 104;
    public const int CurtainRecordSize = 24;
    public const int DhmoRecordSize = 644;

    private AreaFile(uint magic, uint version, AreaChunk?[] chunks, AreaBlock[] blocks)
    {
        Magic = magic;
        Version = version;
        Chunks = chunks;
        Blocks = blocks;
    }

    public uint Magic { get; }

    public uint Version { get; }

    public AreaChunk?[] Chunks { get; }

    public AreaBlock[] Blocks { get; }

    public int ChunkCount
    {
        get
        {
            int n = 0;
            foreach (AreaChunk? c in Chunks)
            {
                if (c is not null)
                {
                    n++;
                }
            }

            return n;
        }
    }

    public IEnumerable<AreaChunk> PresentChunks()
    {
        foreach (AreaChunk? c in Chunks)
        {
            if (c is not null)
            {
                yield return c;
            }
        }
    }

    public static string TagName(uint tag) =>
        new string(new[] { (char)(tag >> 24), (char)((tag >> 16) & 0xFF), (char)((tag >> 8) & 0xFF), (char)(tag & 0xFF) });

    public static bool TryParse(byte[] bytes, out AreaFile area, out string error)
    {
        area = null!;
        if (bytes.Length < 8)
        {
            error = "shorter than an area header";
            return false;
        }

        uint magic = BinaryPrimitives.ReadUInt32LittleEndian(bytes);
        if (magic != MagicCompiled && magic != MagicRaw)
        {
            error = "missing aera signature";
            return false;
        }

        if (magic == MagicRaw)
        {
            error = "raw (editor) AREA files are not supported";
            return false;
        }

        uint version = BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(4));
        if (version != 0)
        {
            error = "unsupported area version " + version;
            return false;
        }

        var chunks = new AreaChunk?[AreaChunk.ChunkCount];
        var blocks = new List<AreaBlock>();
        int pos = 8;
        while (pos < bytes.Length)
        {
            if (bytes.Length - pos < 8)
            {
                error = "truncated block header at " + pos;
                return false;
            }

            uint tag = BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(pos));
            int size = (int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(pos + 4));
            if (size < 0 || bytes.Length - pos - 8 < size)
            {
                error = "block " + TagName(tag) + " runs past the end of the file";
                return false;
            }

            var data = new byte[size];
            Array.Copy(bytes, pos + 8, data, 0, size);
            if (tag == TagChunks)
            {
                if (!ParseChunks(data, chunks, out error))
                {
                    return false;
                }
            }
            else
            {
                blocks.Add(new AreaBlock(tag, data));
            }

            pos += 8 + size;
        }

        area = new AreaFile(magic, version, chunks, blocks.ToArray());
        error = string.Empty;
        return true;
    }

    private static bool ParseChunks(byte[] data, AreaChunk?[] chunks, out string error)
    {
        int pos = 0;
        int next = 0;
        while (pos < data.Length)
        {
            if (data.Length - pos < 4)
            {
                error = "truncated chunk header";
                return false;
            }

            uint header = BinaryPrimitives.ReadUInt32LittleEndian(data.AsSpan(pos));
            int skip = (int)(header >> 24);
            int size = (int)(header & 0xFFFFFF);
            int index = next + skip;
            if (index >= AreaChunk.ChunkCount || data.Length - pos - 4 < size)
            {
                error = "chunk " + index + " out of range";
                return false;
            }

            if (!ParseChunk(index, data, pos + 4, size, out AreaChunk chunk, out error))
            {
                return false;
            }

            chunks[index] = chunk;
            next = index + 1;
            pos += 4 + size;
        }

        error = string.Empty;
        return true;
    }

    private static bool ParseChunk(int index, byte[] data, int start, int size, out AreaChunk chunk, out string error)
    {
        chunk = null!;
        if (size < 4)
        {
            error = "chunk " + index + " shorter than its layer mask";
            return false;
        }

        uint flags = BinaryPrimitives.ReadUInt32LittleEndian(data.AsSpan(start));
        int pos = start + 4;
        int end = start + size;
        var layers = new byte[]?[AreaLayerTable.Count];
        for (int layer = 0; layer < AreaLayerTable.Count; layer++)
        {
            if ((flags & (1u << layer)) == 0)
            {
                continue;
            }

            int length = AreaLayerTable.SizeOf(layer);
            if (end - pos < length)
            {
                error = "chunk " + index + " layer " + layer + " runs past the chunk";
                return false;
            }

            if (AreaLayerTable.Entries[layer].Flag != 0)
            {
                var copy = new byte[length];
                Array.Copy(data, pos, copy, 0, length);
                layers[layer] = copy;
            }

            pos += length;
        }

        var blocks = new List<AreaBlock>();
        while (end - pos >= 8)
        {
            uint tag = BinaryPrimitives.ReadUInt32LittleEndian(data.AsSpan(pos));
            int length = (int)BinaryPrimitives.ReadUInt32LittleEndian(data.AsSpan(pos + 4));
            if (length < 0 || end - pos - 8 < length)
            {
                error = "chunk " + index + " block " + TagName(tag) + " runs past the chunk";
                return false;
            }

            var copy = new byte[length];
            Array.Copy(data, pos + 8, copy, 0, length);
            blocks.Add(new AreaBlock(tag, copy));
            pos += 8 + length;
        }

        if (pos != end)
        {
            error = "chunk " + index + " has " + (end - pos) + " trailing bytes";
            return false;
        }

        chunk = new AreaChunk(index, flags, layers, blocks.ToArray());
        error = string.Empty;
        return true;
    }
}
