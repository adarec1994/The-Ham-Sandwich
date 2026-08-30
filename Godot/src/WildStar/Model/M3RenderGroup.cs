namespace WildStar.Model;

public sealed class M3RenderGroup
{
    public const int NoParent = 0xFFFF;
    public const int TransparentFlag = 0x1;
    public const int AxisAlignedBonesFlag = 0x2;

    public M3RenderGroup(int flags, int parent, int bone, M3Track colourMultiply,
                         M3Track colourAdd, M3Track alpha, M3Track blend, M3Track blend2,
                         M3Track visible, M3Track weightedColour, float scalar)
    {
        Flags = flags;
        Parent = parent;
        Bone = bone;
        ColourMultiply = colourMultiply;
        ColourAdd = colourAdd;
        Alpha = alpha;
        Blend = blend;
        Blend2 = blend2;
        Visible = visible;
        WeightedColour = weightedColour;
        Scalar = scalar;
    }

    public int Flags { get; }

    public int Parent { get; }

    public int Bone { get; }

    public M3Track ColourMultiply { get; }

    public M3Track ColourAdd { get; }

    public M3Track Alpha { get; }

    public M3Track Blend { get; }

    public M3Track Blend2 { get; }

    public M3Track Visible { get; }

    public M3Track WeightedColour { get; }

    public float Scalar { get; }

    public bool IsRoot => Parent == NoParent;

    public bool ForcesTransparency => (Flags & TransparentFlag) != 0;

    public bool AxisAlignedBones => (Flags & AxisAlignedBonesFlag) != 0;
}

public readonly struct M3RenderGroupState
{
    public M3RenderGroupState(float[] colourMultiply, float[] colourAdd, float alpha, bool visible)
    {
        ColourMultiply = colourMultiply;
        ColourAdd = colourAdd;
        Alpha = alpha;
        Visible = visible;
    }

    public static M3RenderGroupState Default =>
        new(new[] { 1.0f, 1.0f, 1.0f, 1.0f }, new float[4], 1.0f, true);

    public float[] ColourMultiply { get; }

    public float[] ColourAdd { get; }

    public float Alpha { get; }

    public bool Visible { get; }

    public bool IsOpaque => Alpha >= M3Material.OpaqueInstanceAlpha;
}
