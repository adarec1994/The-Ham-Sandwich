namespace WildStar.Model;

public readonly struct M3Submesh
{
    public const int NoRecord = 0xFFFF;

    public const int NoAlphaTestFlag = 0x01;
    public const int HiddenInMainPassFlag = 0x02;
    public const int HiddenByRendererFlag = 0x04;
    public const int HiddenInShadowPassFlag = 0x08;

    public M3Submesh(int startIndex, int startVertex, int indexCount, int vertexCount,
                     int bonePaletteStart, int bonePaletteCount, int field14,
                     short materialIndex, short field18, short field1A, short field1C,
                     sbyte groupId, byte groupRelated, short field20, short anatomyId,
                     short[] fields24To2E, byte[] colour0, byte[] colour1,
                     byte byte38, byte byte39,
                     float[] boundMin, float[] boundMax, float[] unknownVector)
    {
        StartIndex = startIndex;
        StartVertex = startVertex;
        IndexCount = indexCount;
        VertexCount = vertexCount;
        BonePaletteStart = bonePaletteStart;
        BonePaletteCount = bonePaletteCount;
        Field14 = field14;
        MaterialIndex = materialIndex;
        Field18 = field18;
        Field1A = field1A;
        Field1C = field1C;
        GroupId = groupId;
        GroupRelated = groupRelated;
        Field20 = field20;
        AnatomyId = anatomyId;
        Fields24To2E = fields24To2E;
        Colour0 = colour0;
        Colour1 = colour1;
        Byte38 = byte38;
        Byte39 = byte39;
        BoundMin = boundMin;
        BoundMax = boundMax;
        UnknownVector = unknownVector;
    }

    public static M3Submesh Whole(int indexCount, int vertexCount, int boneCount) =>
        new(0, 0, indexCount, vertexCount, 0, boneCount, 1,
            0, 0, -1, -1, -1, 0xFF, 0, 0,
            new short[6], new byte[4], new byte[4], 0, 0,
            new float[4], new float[4], new float[4]);

    public int StartIndex { get; }

    public int StartVertex { get; }

    public int IndexCount { get; }

    public int VertexCount { get; }

    public int BonePaletteStart { get; }

    public int BonePaletteCount { get; }

    public int Field14 { get; }

    public int MaterialIndex { get; }

    public short Field18 { get; }

    public short Field1A { get; }

    public short Field1C { get; }

    public sbyte GroupId { get; }

    public byte GroupRelated { get; }

    public short Field20 { get; }

    public short AnatomyId { get; }

    public short[] Fields24To2E { get; }

    public byte[] Colour0 { get; }

    public byte[] Colour1 { get; }

    public byte Byte38 { get; }

    public byte Byte39 { get; }

    public float[] BoundMin { get; }

    public float[] BoundMax { get; }

    public float[] UnknownVector { get; }

    public int UvSetCount => Field14;

    public int RenderGroup => unchecked((ushort)Field18);

    public int SelectorIndex => unchecked((ushort)Field1A);

    public int OverlayIndex => unchecked((ushort)Field1C);

    public int GeosetId => unchecked((byte)GroupId) | (GroupRelated << 8);

    public short SortKey => Field20;

    public int Field22 => unchecked((ushort)AnatomyId);

    public int Field24 => Fields24To2E.Length > 0 ? unchecked((ushort)Fields24To2E[0]) : 0;

    public int Field26 => Fields24To2E.Length > 1 ? unchecked((ushort)Fields24To2E[1]) : 0;

    public int Flags => Byte38;

    public int Flags39 => Byte39;

    public float[] AabbMin => BoundMin;

    public float[] AabbMax => BoundMax;

    public float[] DyeParam => UnknownVector;

    public bool HiddenInMainPass => (Byte38 & HiddenInMainPassFlag) != 0;

    public bool NoAlphaTest => (Byte38 & NoAlphaTestFlag) != 0;

    public bool HasSelector => SelectorIndex != NoRecord;

    public bool HasOverlay => OverlayIndex != NoRecord;
}
