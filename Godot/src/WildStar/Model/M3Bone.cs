namespace WildStar.Model;

public sealed class M3Bone
{
    public const int NoParent = 0xFFFF;

    public const int BillboardSphericalAltFlag = 0x0001;
    public const int BillboardAxisXFlag = 0x0002;
    public const int BillboardAxisYFlag = 0x0004;
    public const int BillboardAxisZFlag = 0x0008;
    public const int NoInheritScaleFlag = 0x0010;
    public const int NoInheritRotationFlag = 0x0020;
    public const int NoInheritTranslationFlag = 0x0040;
    public const int NoInheritAnyFlag = 0x0070;
    public const int BillboardSphericalFlag = 0x0080;
    public const int GroundSnapFlag = 0x0100;
    public const int BillboardCameraYFlag = 0x0200;
    public const int NotEvaluatedFlag = 0x0400;
    public const int GroundAlignFlag = 0x2000;
    public const int BillboardCallerFlag = 0x4000;
    public const int SpecialPathMask = 0x428F;
    public const int BillboardMask = BillboardSphericalAltFlag | BillboardAxisXFlag |
                                     BillboardAxisYFlag | BillboardAxisZFlag |
                                     BillboardSphericalFlag | BillboardCameraYFlag |
                                     BillboardCallerFlag;

    public M3Bone(int linkedBone, int parent, int flags, int renderGroup, uint nameHash,
                  M3Track scale, M3Track scaleLayer,
                  M3Track scaleDivisor, M3Track scaleDivisorLayer,
                  M3Track rotation, M3Track rotationLayer,
                  M3Track translation, M3Track translationLayer,
                  float[] bind, float[] inverseBind, float[] bindPosition)
    {
        LinkedBone = linkedBone;
        Parent = parent;
        Flags = flags;
        RenderGroup = renderGroup;
        NameHash = nameHash;
        Scale = scale;
        ScaleLayer = scaleLayer;
        ScaleDivisor = scaleDivisor;
        ScaleDivisorLayer = scaleDivisorLayer;
        Rotation = rotation;
        RotationLayer = rotationLayer;
        Translation = translation;
        TranslationLayer = translationLayer;
        Bind = bind;
        InverseBind = inverseBind;
        BindPosition = bindPosition;
    }

    public int LinkedBone { get; }

    public int Parent { get; }

    public int Flags { get; }

    public int RenderGroup { get; }

    public uint NameHash { get; }

    public M3Track Scale { get; }

    public M3Track ScaleLayer { get; }

    public M3Track ScaleDivisor { get; }

    public M3Track ScaleDivisorLayer { get; }

    public M3Track Rotation { get; }

    public M3Track RotationLayer { get; }

    public M3Track Translation { get; }

    public M3Track TranslationLayer { get; }

    public float[] Bind { get; }

    public float[] InverseBind { get; }

    public float[] BindPosition { get; }

    public bool IsRoot => Parent == NoParent;

    public bool HasName => NameHash != 0;

    public bool InheritsScale => (Flags & NoInheritScaleFlag) == 0;

    public bool InheritsRotation => (Flags & NoInheritRotationFlag) == 0;

    public bool InheritsTranslation => (Flags & NoInheritTranslationFlag) == 0;

    public bool HasLinkedBone => LinkedBone != NoParent;

    public bool IsBillboard => (Flags & BillboardMask) != 0;

    public bool SnapsToGround => (Flags & GroundSnapFlag) != 0;

    public bool AlignsToGround => (Flags & GroundAlignFlag) != 0;

    public bool IsAnimated =>
        Scale.HasKeys || ScaleDivisor.HasKeys || Rotation.HasKeys || Translation.HasKeys ||
        ScaleLayer.HasKeys || ScaleDivisorLayer.HasKeys || RotationLayer.HasKeys ||
        TranslationLayer.HasKeys;
}
