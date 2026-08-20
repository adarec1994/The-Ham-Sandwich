using System;
using System.Buffers.Binary;
using System.Text;

namespace WildStar.Model;

public enum M3Slot
{
    Position = 0,
    Normal = 1,
    Tangent = 2,
    Bitangent = 3,
    BoneIndices = 4,
    BoneWeights = 5,
    Color0 = 6,
    Color1 = 7,
    Uv0 = 8,
    Uv1 = 9,
    Extra = 10,
}

public enum M3FieldType : byte
{
    Float3 = 1,
    Fixed3 = 2,
    PackedOct2 = 3,
    Bytes4 = 4,
    Half2 = 5,
}

public readonly struct M3VertexField
{
    public M3VertexField(M3FieldType type, int offset)
    {
        Type = type;
        Offset = offset;
    }

    public M3FieldType Type { get; }

    public int Offset { get; }

    public bool Present => Offset >= 0;
}

public sealed class M3File
{
    public const int HeaderSize = 0x630;
    public const uint Magic = 0x4D4F444C;
    public const uint Version = 100;
    public const int SlotCount = 11;
    public const int GeometryRecordSize = 200;
    public const int SubmeshRecordSize = 112;
    public const int BoneRecordSize = 352;
    public const int TextureRecordSize = 32;
    public const int MaterialRecordSize = 48;
    public const int MaterialLayerSize = 296;
    public const int AnimationRecordSize = 112;
    public const float PositionScale = 1.0f / 1024.0f;
    public const float RotationScale = 1.0f / 16384.0f;
    public const float WeightScale = 1.0f / 255.0f;
    public const float DivisorEpsilon = 0.0000099999997f;

    private const int SlotBones = 0x180;
    private const int SlotBoneIds = 0x1B0;
    private const int SlotTextures = 0x1C0;
    private const int SlotMaterials = 0x1F0;
    private const int SlotGeometry = 0x250;
    private const int SlotAnimations = 0x010;
    private const int SlotSequenceVariants = 0x080;
    private const int SlotTintBlock = 0x100;
    private const int SlotGeosetLut = 0x200;
    private const int SlotLights = 0x318;
    private const int HeaderScalar178 = 0x178;
    private const int HeaderAabbMin = 0x390;
    private const int HeaderAabbMax = 0x3A0;

    public const int SequenceVariantRecordSize = 48;
    public const int LightRecordSize = 400;

    private const int GeoVertexCount = 0x18;
    private const int GeoStride = 0x1C;
    private const int GeoSlotMask = 0x1E;
    private const int GeoTypes = 0x20;
    private const int GeoOffsets = 0x2B;
    private const int GeoVertexBlob = 0x38;
    private const int GeoIndexNarrow = 0x6D;
    private const int GeoIndexBlob = 0x70;
    private const int GeoSubmeshes = 0x80;

    private const int BoneParent = 0x04;
    private const int BoneFlags = 0x02;
    private const int BoneScaleTrack = 0x10;
    private const int BoneScaleLayerTrack = 0x28;
    private const int BoneScaleDivisorTrack = 0x40;
    private const int BoneScaleDivisorLayerTrack = 0x58;
    private const int BoneRotationTrack = 0x70;
    private const int BoneRotationLayerTrack = 0x88;
    private const int BoneTranslationTrack = 0xA0;
    private const int BoneTranslationLayerTrack = 0xB8;
    private const int BoneBind = 0xD0;
    private const int BoneInverseBind = 0x110;

    private readonly byte[] _bytes;

    private int _vertexData;
    private int _indexData;

    private M3File(byte[] bytes)
    {
        _bytes = bytes;
        Fields = new M3VertexField[SlotCount];
    }

    public int VertexStride { get; private set; }

    public int VertexCount { get; private set; }

    public int IndexCount { get; private set; }

    public int IndexWidth { get; private set; }

    public int GeometryCount { get; private set; }

    public int BoneCount => Bones.Length;

    public int AnimationCount => Animations.Length;

    public M3Animation[] Animations { get; private set; } = Array.Empty<M3Animation>();

    public M3VertexField[] Fields { get; }

    public M3Submesh[] Submeshes { get; private set; } = Array.Empty<M3Submesh>();

    public M3Bone[] Bones { get; private set; } = Array.Empty<M3Bone>();

    public ushort[] BoneMap { get; private set; } = Array.Empty<ushort>();

    public M3Material[] Materials { get; private set; } = Array.Empty<M3Material>();

    public M3Texture[] Textures { get; private set; } = Array.Empty<M3Texture>();

    public M3SequenceVariant[] SequenceVariants { get; private set; } =
        Array.Empty<M3SequenceVariant>();

    public uint[] GeosetLut { get; private set; } = Array.Empty<uint>();

    public M3Light[] Lights { get; private set; } = Array.Empty<M3Light>();

    public float[] AabbMin { get; private set; } = new float[3];

    public float[] AabbMax { get; private set; } = new float[3];

    public bool HasAabb =>
        AabbMin[0] != AabbMax[0] || AabbMin[1] != AabbMax[1] || AabbMin[2] != AabbMax[2];

    public M3Track TintColor { get; private set; } = M3Track.Empty;

    public M3RawTrack TintTrackA { get; private set; } = M3RawTrack.Empty;

    public M3RawTrack TintTrackB { get; private set; } = M3RawTrack.Empty;

    public float Scalar178 { get; private set; }

    public static bool TryParse(byte[] bytes, out M3File file, out string error)
    {
        file = null!;

        if (bytes.Length < HeaderSize)
        {
            error = "shorter than an M3 header";
            return false;
        }

        if (BinaryPrimitives.ReadUInt32LittleEndian(bytes) != Magic)
        {
            error = "missing LDOM signature";
            return false;
        }

        uint version = BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(4));
        if (version != Version)
        {
            error = "unsupported M3 version " + version;
            return false;
        }

        var parsed = new M3File(bytes);

        parsed.ReadAnimations();
        parsed.ReadBones();
        parsed.ReadBoneMap();
        parsed.ReadTextures();
        parsed.ReadMaterials();
        parsed.ReadSequenceVariants();
        parsed.ReadGeosetLut();
        parsed.ReadTintBlock();
        parsed.ReadLights();
        parsed.ReadAabb();

        int geometryCount = parsed.CountAt(SlotGeometry);
        parsed.GeometryCount = geometryCount;
        if (geometryCount == 0)
        {
            error = "no geometry record";
            return false;
        }

        long geometry = HeaderSize + parsed.OffsetAt(SlotGeometry);
        if (geometry < 0 || geometry + GeometryRecordSize > bytes.Length)
        {
            error = "geometry record runs past the end of the file";
            return false;
        }

        long nested = geometry + AlignUp16((long)GeometryRecordSize * geometryCount);

        if (!parsed.ReadGeometry((int)geometry, nested, out error))
        {
            return false;
        }

        file = parsed;
        error = string.Empty;
        return true;
    }

    private bool ReadGeometry(int geometry, long nested, out string error)
    {
        VertexCount = CountAt(geometry + GeoVertexCount);
        VertexStride = _bytes[geometry + GeoStride];
        IndexWidth = _bytes[geometry + GeoIndexNarrow] == 1 ? 2 : 4;

        int mask = BinaryPrimitives.ReadUInt16LittleEndian(_bytes.AsSpan(geometry + GeoSlotMask));

        for (int slot = 0; slot < SlotCount; slot++)
        {
            Fields[slot] = (mask & (1 << slot)) != 0
                ? new M3VertexField((M3FieldType)_bytes[geometry + GeoTypes + slot],
                                    _bytes[geometry + GeoOffsets + slot])
                : new M3VertexField(default, -1);
        }

        if ((mask & 0x300) == 0x100)
        {
            Fields[(int)M3Slot.Uv1] = Fields[(int)M3Slot.Uv0];
        }

        int vertexBytes = CountAt(geometry + GeoVertexBlob);
        _vertexData = (int)(nested + OffsetAt(geometry + GeoVertexBlob));

        int indexBytes = CountAt(geometry + GeoIndexBlob);
        _indexData = (int)(nested + OffsetAt(geometry + GeoIndexBlob));

        if (VertexStride == 0 || vertexBytes == 0)
        {
            error = "geometry has no vertex data";
            return false;
        }

        if (VertexCount == 0 || (long)VertexCount * VertexStride > vertexBytes)
        {
            VertexCount = vertexBytes / VertexStride;
        }

        IndexCount = indexBytes / IndexWidth;

        if (_vertexData < 0 || _indexData < 0 ||
            _vertexData + vertexBytes > _bytes.Length || _indexData + indexBytes > _bytes.Length)
        {
            error = "vertex or index data runs past the end of the file";
            return false;
        }

        ReadSubmeshes(geometry, nested);

        error = string.Empty;
        return true;
    }

    private void ReadSubmeshes(int geometry, long nested)
    {
        int count = CountAt(geometry + GeoSubmeshes);
        long data = nested + OffsetAt(geometry + GeoSubmeshes);

        if (count <= 0 || data < 0 || data + (long)count * SubmeshRecordSize > _bytes.Length)
        {
            return;
        }

        Submeshes = new M3Submesh[count];

        for (int i = 0; i < count; i++)
        {
            int r = (int)data + i * SubmeshRecordSize;
            var modeFlags = new ushort[4];
            var optionalIds = new ushort[4];
            for (int w = 0; w < 4; w++)
            {
                modeFlags[w] = (ushort)U16(r + 0x28 + w * 2);
                optionalIds[w] = (ushort)U16(r + 0x30 + w * 2);
            }

            Submeshes[i] = new M3Submesh(
                CountAt(r),
                CountAt(r + 0x04),
                CountAt(r + 0x08),
                CountAt(r + 0x0C),
                U16(r + 0x16),
                U16(r + 0x10),
                U16(r + 0x12),
                U16(r + 0x20),
                U16(r + 0x1E),
                U16(r + 0x22),
                U16(r + 0x26),
                _bytes[r + 0x38],
                modeFlags,
                optionalIds);
        }
    }

    private void ReadBones()
    {
        int count = CountAt(SlotBones);
        long data = HeaderSize + OffsetAt(SlotBones);

        if (count <= 0 || data < 0 || data + (long)count * BoneRecordSize > _bytes.Length)
        {
            return;
        }

        long nested = data + AlignUp16((long)count * BoneRecordSize);

        Bones = new M3Bone[count];

        for (int i = 0; i < count; i++)
        {
            int r = (int)data + i * BoneRecordSize;

            Bones[i] = new M3Bone(
                U16(r),
                U16(r + BoneParent),
                U16(r + BoneFlags),
                ReadTrack(r + BoneScaleTrack, nested, M3TrackKind.Half3),
                ReadTrack(r + BoneScaleLayerTrack, nested, M3TrackKind.Half3),
                ReadTrack(r + BoneScaleDivisorTrack, nested, M3TrackKind.Half3),
                ReadTrack(r + BoneScaleDivisorLayerTrack, nested, M3TrackKind.Half3),
                ReadTrack(r + BoneRotationTrack, nested, M3TrackKind.Quaternion16),
                ReadTrack(r + BoneRotationLayerTrack, nested, M3TrackKind.Quaternion16),
                ReadTrack(r + BoneTranslationTrack, nested, M3TrackKind.Float3),
                ReadTrack(r + BoneTranslationLayerTrack, nested, M3TrackKind.Float3),
                Matrix(r + BoneBind),
                Matrix(r + BoneInverseBind));
        }
    }

    private void ReadBoneMap()
    {
        int count = CountAt(SlotBoneIds);
        long data = HeaderSize + OffsetAt(SlotBoneIds);

        if (count <= 0 || data < 0 || data + (long)count * 2 > _bytes.Length)
        {
            return;
        }

        BoneMap = new ushort[count];
        for (int i = 0; i < count; i++)
        {
            BoneMap[i] = (ushort)U16((int)data + i * 2);
        }
    }

    private void ReadTextures()
    {
        int count = CountAt(SlotTextures);
        long data = HeaderSize + OffsetAt(SlotTextures);

        if (count <= 0 || data < 0 || data + (long)count * TextureRecordSize > _bytes.Length)
        {
            return;
        }

        long nested = data + AlignUp16((long)count * TextureRecordSize);

        Textures = new M3Texture[count];

        for (int i = 0; i < count; i++)
        {
            int r = (int)data + i * TextureRecordSize;
            Textures[i] = new M3Texture(
                U16(r),
                U16(r + 2),
                BinaryPrimitives.ReadUInt32LittleEndian(_bytes.AsSpan(r + 4)),
                Utf16(nested, r + 0x10));
        }
    }

    private void ReadMaterials()
    {
        int count = CountAt(SlotMaterials);
        long data = HeaderSize + OffsetAt(SlotMaterials);

        if (count <= 0 || data < 0 || data + (long)count * MaterialRecordSize > _bytes.Length)
        {
            return;
        }

        long nested = data + AlignUp16((long)count * MaterialRecordSize);

        Materials = new M3Material[count];

        for (int i = 0; i < count; i++)
        {
            int r = (int)data + i * MaterialRecordSize;
            int layerCount = CountAt(r + 0x20);
            long layers = nested + OffsetAt(r + 0x20);

            if (layerCount <= 0 || layers < 0 ||
                layers + (long)layerCount * MaterialLayerSize > _bytes.Length)
            {
                Materials[i] = new M3Material(Array.Empty<M3MaterialLayer>());
                continue;
            }

            var built = new M3MaterialLayer[layerCount];
            for (int k = 0; k < layerCount; k++)
            {
                int l = (int)layers + k * MaterialLayerSize;
                built[k] = new M3MaterialLayer(U16(l), U16(l + 2));
            }

            Materials[i] = new M3Material(built);
        }
    }

    private string Utf16(long nested, int arrayAt)
    {
        int count = CountAt(arrayAt);
        long data = nested + OffsetAt(arrayAt);

        if (count <= 0 || data < 0 || data + (long)count * 2 > _bytes.Length)
        {
            return string.Empty;
        }

        string text = Encoding.Unicode.GetString(_bytes, (int)data, count * 2);
        int end = text.IndexOf('\0');
        return end >= 0 ? text[..end] : text;
    }

    private float[] Matrix(int at)
    {
        var values = new float[16];

        if (at < 0 || at + 64 > _bytes.Length)
        {
            values[0] = values[5] = values[10] = values[15] = 1.0f;
            return values;
        }

        for (int i = 0; i < 16; i++)
        {
            values[i] = BitConverter.ToSingle(_bytes, at + i * 4);
        }

        return values;
    }

    private enum M3TrackKind
    {
        Half3,
        Quaternion16,
        Float3,
        Color4,
    }

    private M3Track ReadTrack(int track, long nested, M3TrackKind kind)
    {
        int count = CountAt(track);
        if (count <= 0)
        {
            return M3Track.Empty;
        }

        int stride = kind is M3TrackKind.Quaternion16 or M3TrackKind.Color4 ? 4 : 3;
        int valueSize = kind switch
        {
            M3TrackKind.Half3 => 6,
            M3TrackKind.Quaternion16 => 8,
            M3TrackKind.Color4 => 4,
            _ => 12,
        };

        long keys = nested +
            (long)BinaryPrimitives.ReadUInt64LittleEndian(_bytes.AsSpan(track + 0x08));
        long values = nested +
            (long)BinaryPrimitives.ReadUInt64LittleEndian(_bytes.AsSpan(track + 0x10));

        if (keys < 0 || values < 0 ||
            keys + (long)count * 4 > _bytes.Length ||
            values + (long)count * valueSize > _bytes.Length)
        {
            return M3Track.Empty;
        }

        var times = new uint[count];
        for (int i = 0; i < count; i++)
        {
            times[i] = BinaryPrimitives.ReadUInt32LittleEndian(_bytes.AsSpan((int)keys + i * 4));
        }

        var decoded = new float[count * stride];
        for (int i = 0; i < count; i++)
        {
            int at = (int)values + i * valueSize;

            for (int c = 0; c < stride; c++)
            {
                decoded[i * stride + c] = kind switch
                {
                    M3TrackKind.Color4 => _bytes[at + c] * WeightScale,
                    M3TrackKind.Half3 => HalfBits.ToSingle(
                        BinaryPrimitives.ReadUInt16LittleEndian(_bytes.AsSpan(at + c * 2))),
                    M3TrackKind.Quaternion16 => BinaryPrimitives.ReadInt16LittleEndian(
                        _bytes.AsSpan(at + c * 2)) * RotationScale,
                    _ => BitConverter.ToSingle(_bytes, at + c * 4),
                };
            }
        }

        return new M3Track(times, decoded, stride);
    }

    private void ReadAnimations()
    {
        int count = CountAt(SlotAnimations);
        long data = HeaderSize + OffsetAt(SlotAnimations);

        if (count <= 0 || data < 0 || data + (long)count * AnimationRecordSize > _bytes.Length)
        {
            return;
        }

        long nested = data + AlignUp16((long)count * AnimationRecordSize);
        Animations = new M3Animation[count];

        for (int i = 0; i < count; i++)
        {
            int r = (int)data + i * AnimationRecordSize;
            Animations[i] = new M3Animation(
                U16(r),
                U16(r + 2),
                BinaryPrimitives.ReadUInt32LittleEndian(_bytes.AsSpan(r + 0x0C)),
                BinaryPrimitives.ReadUInt32LittleEndian(_bytes.AsSpan(r + 0x10)),
                ReadU16Array(r + 0x60, nested),
                U16(r + 0x08),
                U16(r + 0x0A),
                BitConverter.ToSingle(_bytes, r + 0x14));
        }
    }

    private ushort[] ReadU16Array(int slot, long nested)
    {
        int count = CountAt(slot);
        long data = nested + (long)BinaryPrimitives.ReadUInt64LittleEndian(_bytes.AsSpan(slot + 8));

        if (count <= 0 || data < 0 || data + (long)count * 2 > _bytes.Length)
        {
            return Array.Empty<ushort>();
        }

        var values = new ushort[count];
        for (int i = 0; i < count; i++)
        {
            values[i] = (ushort)U16((int)data + i * 2);
        }

        return values;
    }

    private void ReadSequenceVariants()
    {
        int count = CountAt(SlotSequenceVariants);
        long data = HeaderSize + OffsetAt(SlotSequenceVariants);

        if (count <= 0 || data < 0 ||
            data + (long)count * SequenceVariantRecordSize > _bytes.Length)
        {
            return;
        }

        SequenceVariants = new M3SequenceVariant[count];

        for (int i = 0; i < count; i++)
        {
            int r = (int)data + i * SequenceVariantRecordSize;
            SequenceVariants[i] = new M3SequenceVariant(
                U16(r),
                U16(r + 0x02),
                U16(r + 0x06),
                (uint)CountAt(r + 0x08),
                (uint)CountAt(r + 0x0C),
                BitConverter.ToSingle(_bytes, r + 0x10),
                (uint)CountAt(r + 0x14),
                (uint)CountAt(r + 0x18));
        }
    }

    private void ReadGeosetLut()
    {
        int count = CountAt(SlotGeosetLut);
        long data = HeaderSize + OffsetAt(SlotGeosetLut);

        if (count <= 0 || data < 0 || data + (long)count * 4 > _bytes.Length)
        {
            return;
        }

        GeosetLut = new uint[count];
        for (int i = 0; i < count; i++)
        {
            GeosetLut[i] = BinaryPrimitives.ReadUInt32LittleEndian(_bytes.AsSpan((int)data + i * 4));
        }
    }

    private void ReadTintBlock()
    {

        TintTrackA = ReadRawTrack(SlotTintBlock + 0x30, HeaderSize, 1);
        TintTrackB = ReadRawTrack(SlotTintBlock + 0x48, HeaderSize, 1);
        TintColor = ReadTrack(SlotTintBlock + 0x60, HeaderSize, M3TrackKind.Color4);
        Scalar178 = BitConverter.ToSingle(_bytes, HeaderScalar178);
    }

    private void ReadLights()
    {
        int count = CountAt(SlotLights);
        long data = HeaderSize + OffsetAt(SlotLights);

        if (count <= 0 || data < 0 || data + (long)count * LightRecordSize > _bytes.Length)
        {
            return;
        }

        ReadOnlySpan<byte> valueSizes = stackalloc byte[M3Light.TrackCount]
            { 2, 2, 2, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };

        long nested = data + AlignUp16((long)count * LightRecordSize);
        Lights = new M3Light[count];

        for (int i = 0; i < count; i++)
        {
            int r = (int)data + i * LightRecordSize;
            var tracks = new M3RawTrack[M3Light.TrackCount];

            for (int t = 0; t < M3Light.TrackCount; t++)
            {
                tracks[t] = ReadRawTrack(r + 0x10 + t * 0x18, nested, valueSizes[t]);
            }

            Lights[i] = new M3Light(_bytes.AsSpan(r, LightRecordSize).ToArray(), tracks);
        }
    }

    private void ReadAabb()
    {
        for (int c = 0; c < 3; c++)
        {
            AabbMin[c] = BitConverter.ToSingle(_bytes, HeaderAabbMin + c * 4);
            AabbMax[c] = BitConverter.ToSingle(_bytes, HeaderAabbMax + c * 4);
        }
    }

    private M3RawTrack ReadRawTrack(int track, long nested, int valueSize)
    {
        int count = CountAt(track);
        if (count <= 0)
        {
            return M3RawTrack.Empty;
        }

        long keys = nested +
            (long)BinaryPrimitives.ReadUInt64LittleEndian(_bytes.AsSpan(track + 0x08));
        long values = nested +
            (long)BinaryPrimitives.ReadUInt64LittleEndian(_bytes.AsSpan(track + 0x10));

        if (keys < 0 || values < 0 ||
            keys + (long)count * 4 > _bytes.Length ||
            values + (long)count * valueSize > _bytes.Length)
        {
            return M3RawTrack.Empty;
        }

        var times = new uint[count];
        for (int i = 0; i < count; i++)
        {
            times[i] = BinaryPrimitives.ReadUInt32LittleEndian(_bytes.AsSpan((int)keys + i * 4));
        }

        return new M3RawTrack(times, _bytes.AsSpan((int)values, count * valueSize).ToArray(),
                              valueSize);
    }

    public ReadOnlySpan<byte> Vertex(int index) =>
        _bytes.AsSpan(_vertexData + index * VertexStride, VertexStride);

    public int Index(int index)
    {
        int at = _indexData + index * IndexWidth;
        return IndexWidth == 2
            ? BinaryPrimitives.ReadUInt16LittleEndian(_bytes.AsSpan(at))
            : (int)BinaryPrimitives.ReadUInt32LittleEndian(_bytes.AsSpan(at));
    }

    public const int NoGeosetKey = 0;
    public const int UngatedGeoset = -1;

    public int GeosetKey(int geosetId)
    {
        if (geosetId < 0 || geosetId >= GeosetLut.Length)
        {
            return UngatedGeoset;
        }

        return (int)(GeosetLut[geosetId] & 0xFFFF);
    }

    public bool GeosetIsGated(int geosetId) => GeosetKey(geosetId) > 0;

    public int[] GeosetKeys()
    {
        var seen = new System.Collections.Generic.SortedSet<int>();
        foreach (M3Submesh submesh in Submeshes)
        {
            int key = GeosetKey(submesh.GeosetId);
            if (key > 0)
            {
                seen.Add(key);
            }
        }

        var keys = new int[seen.Count];
        seen.CopyTo(keys);
        return keys;
    }

    public const int NoTextureSlot = 0xFFFF;

    public bool HasSubstitutableTextures
    {
        get
        {
            foreach (M3Texture texture in Textures)
            {
                if (texture.Slot != NoTextureSlot)
                {
                    return true;
                }
            }

            return false;
        }
    }

    public int[] TextureSlots()
    {
        var seen = new System.Collections.Generic.SortedSet<int>();
        foreach (M3Texture texture in Textures)
        {
            if (texture.Slot != NoTextureSlot)
            {
                seen.Add(texture.Slot);
            }
        }

        var slots = new int[seen.Count];
        seen.CopyTo(slots);
        return slots;
    }

    public int ResolveBone(in M3Submesh submesh, int packed)
    {
        if (BoneMap.Length == 0)
        {
            return packed < Bones.Length ? packed : 0;
        }

        int slot = submesh.BonePaletteStart + packed;
        return slot >= 0 && slot < BoneMap.Length && BoneMap[slot] < Bones.Length
            ? BoneMap[slot]
            : 0;
    }

    public int ResolveBone(int packed)
    {
        if (BoneMap.Length == 0)
        {
            return packed < Bones.Length ? packed : 0;
        }

        return packed < BoneMap.Length && BoneMap[packed] < Bones.Length ? BoneMap[packed] : 0;
    }

    private static long AlignUp16(long value) => (value + 15) & ~15L;

    private int U16(int offset) =>
        BinaryPrimitives.ReadUInt16LittleEndian(_bytes.AsSpan(offset));

    private int CountAt(int offset) =>
        (int)BinaryPrimitives.ReadUInt32LittleEndian(_bytes.AsSpan(offset));

    private long OffsetAt(int offset) =>
        (long)BinaryPrimitives.ReadUInt64LittleEndian(_bytes.AsSpan(offset + 8));
}
