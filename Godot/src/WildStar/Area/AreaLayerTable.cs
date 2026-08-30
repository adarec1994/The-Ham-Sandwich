namespace WildStar.Area;

public readonly struct AreaLayerInfo
{
    public AreaLayerInfo(int id, int kind, int field2, int flag, int cellsX, int cellsY, int x0, int y0,
                         int x1, int y1, int strideX, int strideY, int linkA, int linkB)
    {
        Id = id;
        Kind = kind;
        Field2 = field2;
        Flag = flag;
        CellsX = cellsX;
        CellsY = cellsY;
        X0 = x0;
        Y0 = y0;
        X1 = x1;
        Y1 = y1;
        StrideX = strideX;
        StrideY = strideY;
        LinkA = linkA;
        LinkB = linkB;
    }

    public int Id { get; }

    public int Kind { get; }

    public int Field2 { get; }

    public int Flag { get; }

    public int CellsX { get; }

    public int CellsY { get; }

    public int X0 { get; }

    public int Y0 { get; }

    public int X1 { get; }

    public int Y1 { get; }

    public int StrideX { get; }

    public int StrideY { get; }

    public int LinkA { get; }

    public int LinkB { get; }

    public int Width => X1 - X0;

    public int Height => Y1 - Y0;

    public int Size => StrideY * Height;

    public int Offset(int x, int y) => (x - X0) * StrideX + (y - Y0) * StrideY;
}

public static class AreaLayerTable
{
    public const int Count = 32;

    public const int Heights = 0;
    public const int TextureIds = 1;
    public const int BlendSource = 2;
    public const int ColourSource = 3;
    public const int Layer4 = 4;
    public const int ZoneId = 5;
    public const int SkyIds = 6;
    public const int SkyWeights = 7;
    public const int Layer8 = 8;
    public const int LowHeights = 9;
    public const int HeightBounds = 10;
    public const int Layer11 = 11;
    public const int Layer12 = 12;
    public const int ColourMap = 13;
    public const int ExtraMap = 14;
    public const int Layer15 = 15;
    public const int CellFlags = 16;
    public const int BlendMap = 17;
    public const int BlendFixMap = 18;
    public const int BlendMapB = 19;
    public const int BlendFixMapB = 20;
    public const int Layer21 = 21;
    public const int TextureIdsB = 22;
    public const int Layer23 = 23;
    public const int Layer24 = 24;
    public const int Layer25 = 25;
    public const int Layer26 = 26;
    public const int Layer27 = 27;
    public const int ZoneIds = 28;
    public const int Layer29 = 29;
    public const int Layer30 = 30;
    public const int Layer31 = 31;

    public static readonly AreaLayerInfo[] Entries =
    {
        new(0, 4, 1, 1, 16, 16, -1, -1, 18, 18, 2, 38, 38, 38),
        new(1, 14, 0, 1, 1, 1, 0, 0, 1, 1, 16, 16, 38, 38),
        new(2, 6, 2, 1, 64, 64, 0, 0, 65, 65, 2, 130, 17, 18),
        new(3, 5, 2, 1, 64, 64, 0, 0, 65, 65, 2, 130, 13, 38),
        new(4, 7, 2, 2, 64, 64, 0, 0, 65, 65, 2, 130, 38, 38),
        new(5, 8, 0, 1, 1, 1, 0, 0, 1, 1, 4, 4, 38, 38),
        new(6, 14, 0, 1, 1, 1, 0, 0, 2, 2, 16, 32, 38, 38),
        new(7, 11, 0, 1, 1, 1, 0, 0, 2, 2, 4, 8, 38, 38),
        new(8, 0, 2, 2, 64, 64, 0, 0, 65, 65, 1, 65, 38, 38),
        new(9, 2, 4, 2, 32, 32, 0, 0, 33, 33, 2, 66, 38, 38),
        new(10, 10, 3, 2, 1, 1, 0, 0, 1, 1, 4, 4, 38, 38),
        new(11, 2, 1, 2, 16, 16, 0, 0, 17, 17, 2, 34, 38, 38),
        new(12, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 38, 38),
        new(13, 16, 1, 2, 16, 16, 0, 0, 17, 17, 16, 272, 38, 38),
        new(14, 15, 1, 2, 16, 16, 0, 0, 17, 17, 8, 136, 38, 38),
        new(15, 5, 2, 1, 64, 64, 0, 0, 65, 65, 2, 130, 14, 38),
        new(16, 0, 2, 1, 64, 64, 0, 0, 64, 64, 1, 64, 38, 38),
        new(17, 15, 1, 2, 16, 16, 0, 0, 17, 17, 8, 136, 38, 38),
        new(18, 15, 1, 2, 16, 16, 0, 0, 17, 17, 8, 136, 38, 38),
        new(19, 15, 1, 2, 16, 16, 0, 0, 17, 17, 8, 136, 38, 38),
        new(20, 15, 1, 2, 16, 16, 0, 0, 17, 17, 8, 136, 38, 38),
        new(21, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 38, 38),
        new(22, 14, 0, 2, 1, 1, 0, 0, 1, 1, 16, 16, 38, 38),
        new(23, 11, 2, 1, 64, 64, 0, 0, 65, 65, 4, 260, 38, 38),
        new(24, 2, 0, 1, 1, 1, 0, 0, 2, 2, 2, 4, 38, 38),
        new(25, 2, 2, 2, 64, 64, 0, 0, 65, 65, 2, 130, 38, 38),
        new(26, 11, 2, 1, 64, 64, -4, -4, 69, 69, 4, 292, 38, 38),
        new(27, 0, 2, 1, 64, 64, 0, 0, 64, 64, 1, 64, 38, 38),
        new(28, 14, 0, 1, 1, 1, 0, 0, 1, 1, 16, 16, 38, 38),
        new(29, 6, 2, 1, 64, 64, 0, 0, 65, 65, 2, 130, 19, 20),
        new(30, 5, 2, 1, 64, 64, 0, 0, 65, 65, 2, 130, 31, 38),
        new(31, 15, 1, 2, 16, 16, 0, 0, 17, 17, 8, 136, 38, 38),
    };

    public static int SizeOf(int layer) => Entries[layer].Size;
}
