namespace WildStar.Model;

public readonly struct M3Attachment
{
    public M3Attachment(int id, int bone)
    {
        Id = id;
        Bone = bone;
    }

    public int Id { get; }

    public int Bone { get; }
}

public readonly struct M3Event
{
    public const int TypeSpawnEffect = 0;
    public const int TypeSubObject = 1;
    public const int TypeGroundContact = 2;

    public M3Event(int type, int bone, int paramA, int paramB)
    {
        Type = type;
        Bone = bone;
        ParamA = paramA;
        ParamB = paramB;
    }

    public int Type { get; }

    public int Bone { get; }

    public int ParamA { get; }

    public int ParamB { get; }
}

public sealed class M3Selector
{
    public M3Selector(int bone, int listIndex, M3RawTrack track)
    {
        Bone = bone;
        ListIndex = listIndex;
        Track = track;
    }

    public int Bone { get; }

    public int ListIndex { get; }

    public M3RawTrack Track { get; }
}

public sealed class M3Overlay
{
    public const int KindBlendOverride = 2;
    public const int KindMaterialOverride = 3;

    public M3Overlay(int bone, int kind, int listIndex, int materialIndex, int flags, int id,
                     int blend, M3RawTrack intensity)
    {
        Bone = bone;
        Kind = kind;
        ListIndex = listIndex;
        MaterialIndex = materialIndex;
        Flags = flags;
        Id = id;
        Blend = blend;
        Intensity = intensity;
    }

    public int Bone { get; }

    public int Kind { get; }

    public int ListIndex { get; }

    public int MaterialIndex { get; }

    public int Flags { get; }

    public int Id { get; }

    public int Blend { get; }

    public M3RawTrack Intensity { get; }
}

public readonly struct M3RandomParam
{
    public M3RandomParam(float min, float max)
    {
        Min = min;
        Max = max;
    }

    public float Min { get; }

    public float Max { get; }
}
