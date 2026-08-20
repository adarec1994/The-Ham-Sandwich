using System;

namespace WildStar.Model;

public readonly struct M3Submesh
{
    public const int AlwaysHiddenMask = 0x0A;

    public M3Submesh(int startIndex, int startVertex, int indexCount, int vertexCount,
                     int materialIndex, int bonePaletteStart, int bonePaletteCount,
                     int renderKey, int geosetId, int lodIndex, int lodThreshold, int flags,
                     ushort[] modeFlags, ushort[] optionalIds)
    {
        StartIndex = startIndex;
        StartVertex = startVertex;
        IndexCount = indexCount;
        VertexCount = vertexCount;
        MaterialIndex = materialIndex;
        BonePaletteStart = bonePaletteStart;
        BonePaletteCount = bonePaletteCount;
        RenderKey = renderKey;
        GeosetId = geosetId;
        LodIndex = lodIndex;
        LodThreshold = lodThreshold;
        Flags = flags;
        ModeFlags = modeFlags;
        OptionalIds = optionalIds;
    }

    public int StartIndex { get; }

    public int StartVertex { get; }

    public int IndexCount { get; }

    public int VertexCount { get; }

    public int MaterialIndex { get; }

    public int BonePaletteStart { get; }

    public int BonePaletteCount { get; }

    public int GroupId => BonePaletteStart;

    public int RenderKey { get; }

    public int GeosetId { get; }

    public int VisibilityIndex => GeosetId;

    public int LodIndex { get; }

    public int LodThreshold { get; }

    public int Flags { get; }

    public ushort[] ModeFlags { get; }

    public ushort[] OptionalIds { get; }

    public bool AlwaysHidden => (Flags & AlwaysHiddenMask) == AlwaysHiddenMask;
}
