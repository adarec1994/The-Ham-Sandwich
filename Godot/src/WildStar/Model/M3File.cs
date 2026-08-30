using System;
using System.Buffers.Binary;
using System.Text;

namespace WildStar.Model;

public enum M3Slot
{
    Position = 0,

    Tangent = 1,
    Normal = 2,
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
    public const int SequenceVariantRecordSize = 48;
    public const int RenderGroupRecordSize = 184;
    public const int LightRecordSize = 400;
    public const int AttachmentRecordSize = 4;
    public const int EventRecordSize = 8;
    public const int SelectorRecordSize = 32;
    public const int OverlayRecordSize = 160;
    public const int RandomParamCount = 10;
    public const float PositionScale = 1.0f / 1024.0f;
    public const float RotationScale = 1.0f / 16384.0f;
    public const float WeightScale = 1.0f / 255.0f;
    public const float DivisorEpsilon = 0.0000099999997f;

    private const int SlotAnimations = 0x010;
    private const int SlotSequenceVariants = 0x080;
    private const int SlotRenderGroups = 0x0F0;
    private const int SlotModelColourMultiply = 0x100;
    private const int SlotModelColourAdd = 0x118;
    private const int SlotModelAlpha = 0x130;
    private const int SlotModelBlend = 0x148;
    private const int SlotModelWeightedColour = 0x160;
    private const int HeaderModelScalar = 0x178;
    private const int SlotBones = 0x180;
    private const int SlotBonesByHash = 0x1A0;
    private const int SlotBoneIds = 0x1B0;
    private const int SlotTextures = 0x1C0;
    private const int SlotTextureSlotMap = 0x1D0;
    private const int SlotMaterials = 0x1F0;
    private const int SlotGeosetLut = 0x200;
    private const int SlotGeosetKeyMap = 0x210;
    private const int SlotGeometry = 0x250;
    private const int SlotAttachments = 0x260;
    private const int SlotAttachmentIndex = 0x270;
    private const int SlotEvents = 0x280;
    private const int SlotEventTrack = 0x290;
    private const int SlotParticleEmitters = 0x2F8;
    private const int SlotLights = 0x318;
    private const int HeaderColourTrackABone = 0x348;
    private const int SlotColourTrackA = 0x350;
    private const int HeaderColourTrackBBone = 0x368;
    private const int SlotColourTrackB = 0x370;
    private const int HeaderAabbMin = 0x390;
    private const int HeaderAabbMax = 0x3A0;
    private const int HeaderCullMin = 0x410;
    private const int HeaderCullMax = 0x420;
    private const int SlotOverlays = 0x560;
    private const int SlotSelectors = 0x570;
    private const int HeaderRecord588Bone = 0x580;
    private const int HeaderRandomParams = 0x5D8;

    private const int GeoVertexCount = 0x18;
    private const int GeoStride = 0x1C;
    private const int GeoSlotMask = 0x1E;
    private const int GeoTypes = 0x20;
    private const int GeoOffsets = 0x2B;
    private const int GeoVertexBlob = 0x38;
    private const int GeoIndexCount = 0x68;
    private const int GeoIndexSize = 0x6C;
    private const int GeoIndexFlags = 0x6D;
    private const int GeoIndexBlob = 0x70;
    private const int GeoSubmeshes = 0x80;
    private const int GeoSubmeshVertexStarts = 0x98;
    private const int GeoTriangleTable = 0xA8;
    private const int GeoSubmeshTriangleStarts = 0xB8;

    private const int BoneFlags = 0x02;
    private const int BoneParent = 0x04;
    private const int BoneRenderGroup = 0x06;
    private const int BoneNameHash = 0x08;
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
    private const int BoneBindPosition = 0x150;

    private static readonly int[] LayerChannelTracks = { 0x18, 0x30, 0x48, 0x60, 0x78 };
    private static readonly int[] LayerExtraTracks = { 0xA8, 0xC0, 0xD8, 0xF0 };
    private const int LayerGates = 0x90;
    private const int LayerColourTrack = 0x108;
    private const int LayerTextureTiles = 0x120;
    private const int LayerMaterialTypeId = 0x124;

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

    public int IndexFlags { get; private set; }

    public int GeometryCount { get; private set; }

    public int BoneCount => Bones.Length;

    public int AnimationCount => Animations.Length;

    public M3Animation[] Animations { get; private set; } = Array.Empty<M3Animation>();

    public M3VertexField[] Fields { get; }

    public M3Submesh[] Submeshes { get; private set; } = Array.Empty<M3Submesh>();

    public uint[] SubmeshVertexStarts { get; private set; } = Array.Empty<uint>();

    public uint[] SubmeshTriangleStarts { get; private set; } = Array.Empty<uint>();

    public ushort[] TriangleTable { get; private set; } = Array.Empty<ushort>();

    public M3Bone[] Bones { get; private set; } = Array.Empty<M3Bone>();

    public ushort[] BonesByNameHash { get; private set; } = Array.Empty<ushort>();

    public ushort[] BoneMap { get; private set; } = Array.Empty<ushort>();

    public M3Material[] Materials { get; private set; } = Array.Empty<M3Material>();

    public M3Texture[] Textures { get; private set; } = Array.Empty<M3Texture>();

    public M3ParticleEmitter[] ParticleEmitters { get; private set; } = Array.Empty<M3ParticleEmitter>();

    public ushort[] TextureSlotMap { get; private set; } = Array.Empty<ushort>();

    public M3RenderGroup[] RenderGroups { get; private set; } = Array.Empty<M3RenderGroup>();

    public M3SequenceVariant[] SequenceVariants { get; private set; } =
        Array.Empty<M3SequenceVariant>();

    public uint[] GeosetLut { get; private set; } = Array.Empty<uint>();

    public ushort[] GeosetKeyMap { get; private set; } = Array.Empty<ushort>();

    public M3Attachment[] Attachments { get; private set; } = Array.Empty<M3Attachment>();

    public ushort[] AttachmentIndexById { get; private set; } = Array.Empty<ushort>();

    public M3Event[] Events { get; private set; } = Array.Empty<M3Event>();

    public M3RawTrack EventTrack { get; private set; } = M3RawTrack.Empty;

    public M3Light[] Lights { get; private set; } = Array.Empty<M3Light>();

    public M3Selector[] Selectors { get; private set; } = Array.Empty<M3Selector>();

    public M3Overlay[] Overlays { get; private set; } = Array.Empty<M3Overlay>();

    public M3RandomParam[] RandomParams { get; private set; } = Array.Empty<M3RandomParam>();

    public float[] AabbMin { get; private set; } = new float[3];

    public float[] AabbMax { get; private set; } = new float[3];

    public float[] CullMin { get; private set; } = new float[3];

    public float[] CullMax { get; private set; } = new float[3];

    public bool HasAabb =>
        AabbMin[0] != AabbMax[0] || AabbMin[1] != AabbMax[1] || AabbMin[2] != AabbMax[2];

    public M3Track ModelColourMultiply { get; private set; } = M3Track.Empty;

    public M3Track ModelColourAdd { get; private set; } = M3Track.Empty;

    public M3Track ModelAlpha { get; private set; } = M3Track.Empty;

    public M3Track ModelBlend { get; private set; } = M3Track.Empty;

    public M3Track ModelWeightedColour { get; private set; } = M3Track.Empty;

    public float ModelScalar { get; private set; }

    public int ColourTrackABone { get; private set; } = 0xFFFF;

    public M3Track ColourTrackA { get; private set; } = M3Track.Empty;

    public int ColourTrackBBone { get; private set; } = 0xFFFF;

    public M3Track ColourTrackB { get; private set; } = M3Track.Empty;

    public int Record588Bone { get; private set; } = 0xFFFF;

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
        parsed.ReadRenderGroups();
        parsed.ReadModelState();
        parsed.ReadBones();
        parsed.ReadU16Slot(SlotBonesByHash, v => parsed.BonesByNameHash = v);
        parsed.ReadBoneMap();
        parsed.ReadTextures();
        parsed.ReadU16Slot(SlotTextureSlotMap, v => parsed.TextureSlotMap = v);
        parsed.ReadMaterials();
        parsed.ReadSequenceVariants();
        parsed.ReadGeosetLut();
        parsed.ReadU16Slot(SlotGeosetKeyMap, v => parsed.GeosetKeyMap = v);
        parsed.ReadAttachments();
        parsed.ReadEvents();
        parsed.ReadLights();
        parsed.ReadParticleEmitters();
        parsed.ReadSelectors();
        parsed.ReadOverlays();
        parsed.ReadRandomParams();
        parsed.ReadBounds();

        uint rawGeometryCount = BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(SlotGeometry));
        if (rawGeometryCount > int.MaxValue)
        {
            error = "geometry record count is too large";
            return false;
        }

        int geometryCount = (int)rawGeometryCount;
        parsed.GeometryCount = geometryCount;
        if (geometryCount == 0)
        {
            file = parsed;
            error = string.Empty;
            return true;
        }

        if (!parsed.TryResolveArray(SlotGeometry, HeaderSize, GeometryRecordSize,
                                    out _, out int geometry))
        {
            error = "geometry table runs past the end of the file";
            return false;
        }

        long nested = geometry + AlignUp16((long)GeometryRecordSize * geometryCount);
        if (nested < geometry || nested > bytes.Length)
        {
            error = "geometry nested-data base runs past the end of the file";
            return false;
        }

        if (!parsed.ReadGeometry(geometry, nested, out error))
        {
            return false;
        }

        parsed.HasGeometry = parsed.VertexCount != 0 && parsed.IndexCount != 0;
        file = parsed;
        error = string.Empty;
        return true;
    }

    public bool HasGeometry { get; private set; }

    private bool ReadGeometry(int geometry, long nested, out string error)
    {
        uint rawVertexCount = BinaryPrimitives.ReadUInt32LittleEndian(
            _bytes.AsSpan(geometry + GeoVertexCount));
        uint rawIndexCount = BinaryPrimitives.ReadUInt32LittleEndian(
            _bytes.AsSpan(geometry + GeoIndexCount));
        if (rawVertexCount > int.MaxValue || rawIndexCount > int.MaxValue)
        {
            error = "geometry vertex or index count is too large";
            return false;
        }

        VertexCount = (int)rawVertexCount;
        IndexCount = (int)rawIndexCount;
        VertexStride = _bytes[geometry + GeoStride];
        IndexWidth = _bytes[geometry + GeoIndexSize];
        IndexFlags = _bytes[geometry + GeoIndexFlags];

        ReadOnlySpan<int> arrayOffsets = stackalloc int[]
            { 0x08, 0x38, 0x48, 0x58, 0x70, 0x80, 0x98, 0xA8, 0xB8 };
        ReadOnlySpan<int> elementSizes = stackalloc int[]
            { 4, 1, 4, 4, 1, SubmeshRecordSize, 4, 2, 4 };
        for (int i = 0; i < arrayOffsets.Length; i++)
        {
            if (!TryResolveArray(geometry + arrayOffsets[i], nested, elementSizes[i],
                                 out _, out _))
            {
                error = $"geometry array at +0x{arrayOffsets[i]:X} runs past the end of the file";
                return false;
            }
        }

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

        for (int slot = 0; slot < SlotCount; slot++)
        {
            if (!Fields[slot].Present)
            {
                continue;
            }

            int fieldSize = slot == 0 ? (Fields[slot].Type == M3FieldType.Float3 ? 12 : 6)
                          : slot <= 3 ? 2
                          : slot <= 9 ? 4
                          : 1;
            if (Fields[slot].Offset > VertexStride ||
                fieldSize > VertexStride - Fields[slot].Offset)
            {
                error = $"vertex stream {slot} runs past the declared stride";
                return false;
            }
        }

        if (!TryResolveArray(geometry + GeoVertexBlob, nested, 1,
                             out int vertexBytes, out _vertexData) ||
            !TryResolveArray(geometry + GeoIndexBlob, nested, 1,
                             out int indexBytes, out _indexData))
        {
            error = "vertex or index data runs past the end of the file";
            return false;
        }

        if ((VertexCount != 0 && VertexStride == 0) ||
            (long)VertexCount * VertexStride > vertexBytes)
        {
            error = "declared vertices do not fit in the vertex blob";
            return false;
        }

        if (IndexCount != 0 && IndexWidth != 2 && IndexWidth != 4)
        {
            error = "index element width is neither 2 nor 4 bytes";
            return false;
        }

        if ((long)IndexCount * IndexWidth > indexBytes)
        {
            error = "declared indices do not fit in the index blob";
            return false;
        }

        if (!ReadSubmeshes(geometry, nested, out error))
        {
            return false;
        }
        SubmeshVertexStarts = ReadU32Array(geometry + GeoSubmeshVertexStarts, nested);
        SubmeshTriangleStarts = ReadU32Array(geometry + GeoSubmeshTriangleStarts, nested);
        TriangleTable = ReadU16Array(geometry + GeoTriangleTable, nested);

        error = string.Empty;
        return true;
    }

    private bool ReadSubmeshes(int geometry, long nested, out string error)
    {
        if (!TryResolveArray(geometry + GeoSubmeshes, nested, SubmeshRecordSize,
                             out int count, out int data))
        {
            error = "submesh table runs past the end of the file";
            return false;
        }

        Submeshes = new M3Submesh[count];

        for (int i = 0; i < count; i++)
        {
            int r = data + i * SubmeshRecordSize;
            var fields24To2E = new short[6];
            for (int w = 0; w < fields24To2E.Length; w++)
            {
                fields24To2E[w] = I16(r + 0x24 + w * 2);
            }

            Submeshes[i] = new M3Submesh(
                CountAt(r),
                CountAt(r + 0x04),
                CountAt(r + 0x08),
                CountAt(r + 0x0C),
                U16(r + 0x10),
                U16(r + 0x12),
                U16(r + 0x14),
                I16(r + 0x16),
                I16(r + 0x18),
                I16(r + 0x1A),
                I16(r + 0x1C),
                unchecked((sbyte)_bytes[r + 0x1E]),
                _bytes[r + 0x1F],
                I16(r + 0x20),
                I16(r + 0x22),
                fields24To2E,
                _bytes.AsSpan(r + 0x30, 4).ToArray(),
                _bytes.AsSpan(r + 0x34, 4).ToArray(),
                _bytes[r + 0x38],
                _bytes[r + 0x39],
                Float4(r + 0x40),
                Float4(r + 0x50),
                Float4(r + 0x60));
        }

        error = string.Empty;
        return true;
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
                U16(r + BoneRenderGroup),
                BinaryPrimitives.ReadUInt32LittleEndian(_bytes.AsSpan(r + BoneNameHash)),
                ReadTrack(r + BoneScaleTrack, nested, M3TrackKind.Half3),
                ReadTrack(r + BoneScaleLayerTrack, nested, M3TrackKind.Half3),
                ReadTrack(r + BoneScaleDivisorTrack, nested, M3TrackKind.Half3),
                ReadTrack(r + BoneScaleDivisorLayerTrack, nested, M3TrackKind.Half3),
                ReadTrack(r + BoneRotationTrack, nested, M3TrackKind.Quaternion16),
                ReadTrack(r + BoneRotationLayerTrack, nested, M3TrackKind.Quaternion16),
                ReadTrack(r + BoneTranslationTrack, nested, M3TrackKind.Float3),
                ReadTrack(r + BoneTranslationLayerTrack, nested, M3TrackKind.Float3),
                Matrix(r + BoneBind),
                Matrix(r + BoneInverseBind),
                Float3(r + BoneBindPosition));
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
                BitConverter.ToSingle(_bytes, r + 8),
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

            int bone = CountAt(r);
            uint type = (uint)CountAt(r + 0x08);
            uint flags = (uint)CountAt(r + 0x0C);
            uint op = (uint)CountAt(r + 0x10);
            uint blend = (uint)CountAt(r + 0x14);
            uint rowA = (uint)CountAt(r + 0x18);
            uint rowB = (uint)CountAt(r + 0x1C);

            if (layerCount <= 0 || layers < 0 ||
                layers + (long)layerCount * MaterialLayerSize > _bytes.Length)
            {
                Materials[i] = new M3Material(Array.Empty<M3MaterialLayer>(), bone, type, flags,
                                              op, blend, rowA, rowB);
                continue;
            }

            long layerNested = layers + AlignUp16((long)layerCount * MaterialLayerSize);
            var built = new M3MaterialLayer[layerCount];
            for (int k = 0; k < layerCount; k++)
            {
                int l = (int)layers + k * MaterialLayerSize;

                var sources = new uint[5];
                var gates = new uint[5];
                var channels = new M3Track[5];
                for (int c = 0; c < 5; c++)
                {
                    sources[c] = (uint)CountAt(l + 0x04 + c * 4);
                    gates[c] = (uint)CountAt(l + LayerGates + c * 4);
                    channels[c] = ReadTrack(l + LayerChannelTracks[c], layerNested,
                                            M3TrackKind.Float1);
                }

                var extras = new M3Track[4];
                for (int c = 0; c < 4; c++)
                {
                    extras[c] = ReadTrack(l + LayerExtraTracks[c], layerNested, M3TrackKind.Float1);
                }

                built[k] = new M3MaterialLayer(
                    U16(l), U16(l + 2), sources, gates, channels, extras,
                    ReadTrack(l + LayerColourTrack, layerNested, M3TrackKind.Float3),
                    (uint)CountAt(l + LayerTextureTiles),
                    (uint)CountAt(l + LayerMaterialTypeId));
            }

            Materials[i] = new M3Material(built, bone, type, flags, op, blend, rowA, rowB);
        }
    }

    private void ReadRenderGroups()
    {
        int count = CountAt(SlotRenderGroups);
        long data = HeaderSize + OffsetAt(SlotRenderGroups);

        if (count <= 0 || data < 0 || data + (long)count * RenderGroupRecordSize > _bytes.Length)
        {
            return;
        }

        long nested = data + AlignUp16((long)count * RenderGroupRecordSize);
        RenderGroups = new M3RenderGroup[count];

        for (int i = 0; i < count; i++)
        {
            int r = (int)data + i * RenderGroupRecordSize;
            RenderGroups[i] = new M3RenderGroup(
                U16(r),
                U16(r + 0x02),
                U16(r + 0x04),
                ReadTrack(r + 0x08, nested, M3TrackKind.Color4),
                ReadTrack(r + 0x20, nested, M3TrackKind.Color4),
                ReadTrack(r + 0x38, nested, M3TrackKind.Byte1),
                ReadTrack(r + 0x50, nested, M3TrackKind.Byte1),
                ReadTrack(r + 0x68, nested, M3TrackKind.Byte1),
                ReadTrack(r + 0x80, nested, M3TrackKind.Byte1),
                ReadTrack(r + 0x98, nested, M3TrackKind.Color4),
                BitConverter.ToSingle(_bytes, r + 0xB0));
        }
    }

    private void ReadModelState()
    {
        ModelColourMultiply = ReadTrack(SlotModelColourMultiply, HeaderSize, M3TrackKind.Color4);
        ModelColourAdd = ReadTrack(SlotModelColourAdd, HeaderSize, M3TrackKind.Color4);
        ModelAlpha = ReadTrack(SlotModelAlpha, HeaderSize, M3TrackKind.Byte1);
        ModelBlend = ReadTrack(SlotModelBlend, HeaderSize, M3TrackKind.Byte1);
        ModelWeightedColour = ReadTrack(SlotModelWeightedColour, HeaderSize, M3TrackKind.Color4);
        ModelScalar = BitConverter.ToSingle(_bytes, HeaderModelScalar);

        ColourTrackABone = U16(HeaderColourTrackABone);
        ColourTrackA = ReadTrack(SlotColourTrackA, HeaderSize, M3TrackKind.Color4);
        ColourTrackBBone = U16(HeaderColourTrackBBone);
        ColourTrackB = ReadTrack(SlotColourTrackB, HeaderSize, M3TrackKind.Color4);
        Record588Bone = U16(HeaderRecord588Bone);
    }

    public M3RenderGroupState RenderGroupStateAtRest(int group)
    {
        float[] multiply = { 1.0f, 1.0f, 1.0f, 1.0f };
        float[] add = new float[4];
        float alpha = ModelAlpha.HasKeys ? ModelAlpha.Values[0] : 1.0f;
        bool visible = true;

        if (ModelColourMultiply.HasKeys)
        {
            for (int c = 0; c < 4; c++)
            {
                multiply[c] = ModelColourMultiply.Values[c];
            }
        }

        if (ModelColourAdd.HasKeys)
        {
            for (int c = 0; c < 4; c++)
            {
                add[c] = ModelColourAdd.Values[c];
            }
        }

        var chain = new System.Collections.Generic.List<int>();
        int cursor = group;
        int guard = 0;
        while (cursor >= 0 && cursor < RenderGroups.Length && guard++ < RenderGroups.Length)
        {
            chain.Add(cursor);
            cursor = RenderGroups[cursor].Parent;
        }

        for (int i = chain.Count - 1; i >= 0; i--)
        {
            M3RenderGroup g = RenderGroups[chain[i]];

            if (g.ColourMultiply.HasKeys)
            {
                for (int c = 0; c < 4; c++)
                {
                    multiply[c] *= g.ColourMultiply.Values[c];
                }
            }

            if (g.ColourAdd.HasKeys)
            {
                for (int c = 0; c < 4; c++)
                {
                    add[c] += g.ColourAdd.Values[c];
                }
            }

            if (g.Alpha.HasKeys && alpha != 0.0f)
            {
                alpha *= g.Alpha.Values[0];
            }

            if (g.Visible.HasKeys && visible)
            {
                visible = g.Visible.Values[0] != 0.0f;
            }
        }

        return new M3RenderGroupState(multiply, add, alpha, visible);
    }

    private void ReadAttachments()
    {
        int count = CountAt(SlotAttachments);
        long data = HeaderSize + OffsetAt(SlotAttachments);

        if (count > 0 && data >= 0 && data + (long)count * AttachmentRecordSize <= _bytes.Length)
        {
            Attachments = new M3Attachment[count];
            for (int i = 0; i < count; i++)
            {
                int r = (int)data + i * AttachmentRecordSize;
                Attachments[i] = new M3Attachment(U16(r), U16(r + 2));
            }
        }

        ReadU16Slot(SlotAttachmentIndex, v => AttachmentIndexById = v);
    }

    private void ReadEvents()
    {
        int count = CountAt(SlotEvents);
        long data = HeaderSize + OffsetAt(SlotEvents);

        if (count > 0 && data >= 0 && data + (long)count * EventRecordSize <= _bytes.Length)
        {
            Events = new M3Event[count];
            for (int i = 0; i < count; i++)
            {
                int r = (int)data + i * EventRecordSize;
                Events[i] = new M3Event(U16(r), U16(r + 2), U16(r + 4), U16(r + 6));
            }
        }

        EventTrack = ReadRawTrack(SlotEventTrack, HeaderSize, 2);
    }

    private void ReadSelectors()
    {
        int count = CountAt(SlotSelectors);
        long data = HeaderSize + OffsetAt(SlotSelectors);

        if (count <= 0 || data < 0 || data + (long)count * SelectorRecordSize > _bytes.Length)
        {
            return;
        }

        long nested = data + AlignUp16((long)count * SelectorRecordSize);
        Selectors = new M3Selector[count];
        for (int i = 0; i < count; i++)
        {
            int r = (int)data + i * SelectorRecordSize;
            Selectors[i] = new M3Selector(U16(r), U16(r + 4), ReadRawTrack(r + 8, nested, 1));
        }
    }

    private void ReadOverlays()
    {
        int count = CountAt(SlotOverlays);
        long data = HeaderSize + OffsetAt(SlotOverlays);

        if (count <= 0 || data < 0 || data + (long)count * OverlayRecordSize > _bytes.Length)
        {
            return;
        }

        long nested = data + AlignUp16((long)count * OverlayRecordSize);
        Overlays = new M3Overlay[count];
        for (int i = 0; i < count; i++)
        {
            int r = (int)data + i * OverlayRecordSize;
            Overlays[i] = new M3Overlay(U16(r), U16(r + 0x06), U16(r + 0x0C), U16(r + 0x0E),
                                        U16(r + 0x12), U16(r + 0x14), U16(r + 0x16),
                                        ReadRawTrack(r + 0x28, nested, 2));
        }
    }

    private void ReadRandomParams()
    {
        RandomParams = new M3RandomParam[RandomParamCount];
        for (int pair = 0; pair < RandomParamCount / 2; pair++)
        {
            int at = HeaderRandomParams + pair * 16;
            RandomParams[pair * 2] = new M3RandomParam(
                BitConverter.ToSingle(_bytes, at), BitConverter.ToSingle(_bytes, at + 8));
            RandomParams[pair * 2 + 1] = new M3RandomParam(
                BitConverter.ToSingle(_bytes, at + 4), BitConverter.ToSingle(_bytes, at + 12));
        }
    }

    private void ReadU16Slot(int slot, Action<ushort[]> store)
    {
        ushort[] values = ReadU16Array(slot, HeaderSize);
        if (values.Length != 0)
        {
            store(values);
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

    private float[] Float3(int at)
    {
        var values = new float[3];
        if (at < 0 || at + 12 > _bytes.Length)
        {
            return values;
        }

        for (int i = 0; i < 3; i++)
        {
            values[i] = BitConverter.ToSingle(_bytes, at + i * 4);
        }

        return values;
    }

    private float[] Float4(int at)
    {
        var values = new float[4];
        if (at < 0 || at + 16 > _bytes.Length)
        {
            return values;
        }

        for (int i = 0; i < 4; i++)
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
        Float1,
        Byte1,
    }

    private M3Track ReadTrack(int track, long nested, M3TrackKind kind)
    {
        int count = CountAt(track);
        if (count <= 0)
        {
            return M3Track.Empty;
        }

        int stride = kind switch
        {
            M3TrackKind.Quaternion16 or M3TrackKind.Color4 => 4,
            M3TrackKind.Float1 or M3TrackKind.Byte1 => 1,
            _ => 3,
        };
        int valueSize = kind switch
        {
            M3TrackKind.Half3 => 6,
            M3TrackKind.Quaternion16 => 8,
            M3TrackKind.Color4 => 4,
            M3TrackKind.Float1 => 4,
            M3TrackKind.Byte1 => 1,
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
                    M3TrackKind.Color4 or M3TrackKind.Byte1 => _bytes[at + c] * WeightScale,
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
            int sequenceId = U16(r);
            int variation = i > 0 && Animations[i - 1].SequenceId == sequenceId
                ? Animations[i - 1].Variation + 1
                : 0;
            Animations[i] = new M3Animation(
                sequenceId,
                U16(r + 2),
                variation,
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

    private uint[] ReadU32Array(int slot, long nested)
    {
        int count = CountAt(slot);
        long data = nested + (long)BinaryPrimitives.ReadUInt64LittleEndian(_bytes.AsSpan(slot + 8));

        if (count <= 0 || data < 0 || data + (long)count * 4 > _bytes.Length)
        {
            return Array.Empty<uint>();
        }

        var values = new uint[count];
        for (int i = 0; i < count; i++)
        {
            values[i] = BinaryPrimitives.ReadUInt32LittleEndian(_bytes.AsSpan((int)data + i * 4));
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
        GeosetLut = ReadU32Array(SlotGeosetLut, HeaderSize);
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
            { 2, 2, 2, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };

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

    private void ReadParticleEmitters()
    {
        int count = CountAt(SlotParticleEmitters);
        long data = HeaderSize + OffsetAt(SlotParticleEmitters);
        if (count <= 0 || count > 4096 || data < 0 || data + (long)count * M3ParticleEmitter.RecordSize > _bytes.Length)
        {
            return;
        }

        long nested = data + AlignUp16((long)count * M3ParticleEmitter.RecordSize);
        ParticleEmitters = new M3ParticleEmitter[count];
        for (int i = 0; i < count; i++)
        {
            ParticleEmitters[i] = M3ParticleEmitter.Read(_bytes, (int)data + i * M3ParticleEmitter.RecordSize, nested);
        }
    }

    private void ReadBounds()
    {
        for (int c = 0; c < 3; c++)
        {
            AabbMin[c] = BitConverter.ToSingle(_bytes, HeaderAabbMin + c * 4);
            AabbMax[c] = BitConverter.ToSingle(_bytes, HeaderAabbMax + c * 4);
            CullMin[c] = BitConverter.ToSingle(_bytes, HeaderCullMin + c * 4);
            CullMax[c] = BitConverter.ToSingle(_bytes, HeaderCullMax + c * 4);
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

    public bool GeosetOnByDefault(int geosetId) =>
        geosetId >= 0 && geosetId < GeosetLut.Length && ((GeosetLut[geosetId] >> 16) & 1) != 0;

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

    public int FindBoneByNameHash(uint hash)
    {
        if (hash == 0)
        {
            return -1;
        }

        int low = 0;
        int high = BonesByNameHash.Length;
        while (low < high)
        {
            int mid = (low + high) >> 1;
            int bone = BonesByNameHash[mid];
            uint candidate = bone < Bones.Length ? Bones[bone].NameHash : 0;
            if (candidate == hash)
            {
                return bone;
            }

            if (candidate < hash)
            {
                low = mid + 1;
            }
            else
            {
                high = mid;
            }
        }

        return -1;
    }

    public int AttachmentBone(int attachmentId)
    {
        if (attachmentId < 0 || attachmentId >= AttachmentIndexById.Length)
        {
            return -1;
        }

        int index = AttachmentIndexById[attachmentId];
        return index < Attachments.Length ? Attachments[index].Bone : -1;
    }

    public int ResolveBone(in M3Submesh submesh, int packed)
        => ResolveBone(packed);

    public int ResolveBone(int packed)
    {
        if (BoneMap.Length == 0)
        {
            return packed < Bones.Length ? packed : 0;
        }

        return packed < BoneMap.Length && BoneMap[packed] < Bones.Length ? BoneMap[packed] : 0;
    }

    private static long AlignUp16(long value) => (value + 15) & ~15L;

    private bool TryResolveArray(int descriptor, long nested, int elementSize,
                                 out int count, out int data)
    {
        count = 0;
        data = 0;

        uint rawCount = BinaryPrimitives.ReadUInt32LittleEndian(_bytes.AsSpan(descriptor));
        long relative = BinaryPrimitives.ReadInt64LittleEndian(_bytes.AsSpan(descriptor + 8));
        if (rawCount > int.MaxValue || relative < 0 || nested < 0 ||
            relative > long.MaxValue - nested)
        {
            return false;
        }

        long start = nested + relative;
        long byteCount = (long)rawCount * elementSize;
        if (start < 0 || start > _bytes.Length || byteCount > _bytes.Length - start)
        {
            return false;
        }

        count = (int)rawCount;
        data = (int)start;
        return true;
    }

    private int U16(int offset) =>
        BinaryPrimitives.ReadUInt16LittleEndian(_bytes.AsSpan(offset));

    private short I16(int offset) =>
        BinaryPrimitives.ReadInt16LittleEndian(_bytes.AsSpan(offset));

    private int CountAt(int offset) =>
        (int)BinaryPrimitives.ReadUInt32LittleEndian(_bytes.AsSpan(offset));

    private long OffsetAt(int offset) =>
        BinaryPrimitives.ReadInt64LittleEndian(_bytes.AsSpan(offset + 8));
}
