namespace WildStar.Model;

public sealed class M3Bone
{
    public const int NoParent = 0xFFFF;
    public const int NoInheritScaleFlag = 0x10;
    public const int NoInheritRotationFlag = 0x20;
    public const int NoInheritTranslationFlag = 0x40;
    public const int NoInheritAnyFlag = 0x70;

    public const int LinkedOverrideScaleFlag = 0x01;
    public const int LinkedOverrideRotationFlag = 0x02;
    public const int LinkedOverrideTranslationFlag = 0x04;

    public M3Bone(int linkedBone, int parent, int flags,
                  M3Track scale, M3Track scaleLayer,
                  M3Track scaleDivisor, M3Track scaleDivisorLayer,
                  M3Track rotation, M3Track rotationLayer,
                  M3Track translation, M3Track translationLayer,
                  float[] bind, float[] inverseBind)
    {
        LinkedBone = linkedBone;
        Parent = parent;
        Flags = flags;
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
    }

    public int LinkedBone { get; }

    public int Parent { get; }

    public int Flags { get; }

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

    public bool IsRoot => Parent == NoParent;

    public bool InheritsScale => (Flags & NoInheritScaleFlag) == 0;

    public bool InheritsRotation => (Flags & NoInheritRotationFlag) == 0;

    public bool InheritsTranslation => (Flags & NoInheritTranslationFlag) == 0;

    public bool HasLinkedBone => LinkedBone != NoParent;

    public bool IsAnimated =>
        Scale.HasKeys || ScaleDivisor.HasKeys || Rotation.HasKeys || Translation.HasKeys ||
        ScaleLayer.HasKeys || ScaleDivisorLayer.HasKeys || RotationLayer.HasKeys ||
        TranslationLayer.HasKeys;
}
