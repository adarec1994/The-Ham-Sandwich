namespace WildStar.Model;

public readonly struct M3MaterialLayer
{
    public const int ChannelHeight = 0;
    public const int ChannelOpacity = 1;
    public const int ChannelGloss = 2;
    public const int ChannelGlow = 3;
    public const int ChannelShader = 4;

    public const uint SourceConstant = 0;
    public const uint SourceColourAlpha = 1;
    public const uint SourceColourAlphaInverted = 2;
    public const uint SourceNormalBlue = 3;
    public const uint SourceNormalBlueInverted = 4;
    public const uint SourceNormalRed = 5;
    public const uint SourceNormalRedInverted = 6;

    public const uint DefaultTextureTiles = 3;

    private static readonly float[] ChannelDefaults = { 1.0f, 1.0f, 1.0f, 0.0f, 0.0f };

    public M3MaterialLayer(int textureA, int textureB, uint[] sources, uint[] gates,
                           M3Track[] channelTracks, M3Track[] extraTracks, M3Track colour,
                           uint textureTiles, uint materialTypeId)
    {
        TextureA = textureA;
        TextureB = textureB;
        Sources = sources;
        Gates = gates;
        ChannelTracks = channelTracks;
        ExtraTracks = extraTracks;
        GlowColour = colour;
        TextureTiles = textureTiles;
        MaterialTypeId = materialTypeId;
    }

    public int TextureA { get; }

    public int TextureB { get; }

    public uint[] Sources { get; }

    public uint[] Gates { get; }

    public M3Track[] ChannelTracks { get; }

    public M3Track[] ExtraTracks { get; }

    public M3Track GlowColour { get; }

    public uint TextureTiles { get; }

    public uint MaterialTypeId { get; }

    public uint HeightSource => Sources[ChannelHeight];

    public uint OpacitySource => Sources[ChannelOpacity];

    public uint GlossSource => Sources[ChannelGloss];

    public uint GlowSource => Sources[ChannelGlow];

    public uint ShaderSource => Sources[ChannelShader];

    public float ChannelValue(int channel)
    {
        M3Track track = ChannelTracks[channel];
        return track.HasKeys ? track.Values[0] : ChannelDefaults[channel];
    }

    public float ChannelScale(int channel) =>
        Sources[channel] == SourceConstant || Gates[channel] != 0
            ? ChannelValue(channel)
            : 1.0f;

    public bool OpacityFromColourAlpha =>
        OpacitySource == SourceColourAlpha || OpacitySource == SourceColourAlphaInverted;

    public bool OpacityInverted => OpacitySource == SourceColourAlphaInverted;

    public float OpacityScale => ChannelScale(ChannelOpacity);

    public bool HasAnySource
    {
        get
        {
            foreach (uint source in Sources)
            {
                if (source != 0)
                {
                    return true;
                }
            }

            return false;
        }
    }
}

public sealed class M3Material
{
    public const uint TypeFull = 0;
    public const uint TypeSimple = 1;
    public const uint TypeTransparent = 2;

    public const uint FlagDepthTestAlways = 0x1;
    public const uint FlagNoDepthWrite = 0x2;
    public const uint FlagTwoSided = 0x4;
    public const uint FlagDistortion = 0x200;
    public const uint FlagQualityTableA = 0x2000;
    public const uint FlagQualityTableB = 0x4000;
    public const uint FlagShaderSwitchOff = 0x8000;
    public const uint FlagDepthOnly = 0x10000;

    public const uint BlendOpaque = 0;
    public const uint BlendAlphaTest = 1;
    public const uint BlendAlpha = 2;
    public const uint BlendAdditive = 3;
    public const uint BlendAlphaAdditive = 4;
    public const uint BlendModulate = 5;
    public const uint BlendModulate2X = 6;
    public const uint BlendDecal = 7;
    public const uint BlendSubtract = 8;
    public const uint BlendAdditiveAlt = 9;
    public const uint BlendSoftAdditive = 10;

    public const float AlphaTestReference = 0.5f;
    public const float OpaqueInstanceAlpha = 254.0f / 255.0f;

    public M3Material(M3MaterialLayer[] layers, int boneIndex, uint type, uint flags, uint op,
                      uint blend, uint materialDataRowA, uint materialDataRowB)
    {
        Layers = layers;
        BoneIndex = boneIndex;
        Type = type;
        Flags = flags;
        Op = op;
        Blend = blend;
        MaterialDataRowA = materialDataRowA;
        MaterialDataRowB = materialDataRowB;
    }

    public M3MaterialLayer[] Layers { get; }

    public int BoneIndex { get; }

    public uint Type { get; }

    public uint Flags { get; }

    public uint Op { get; }

    public uint Blend { get; }

    public uint MaterialDataRowA { get; }

    public uint MaterialDataRowB { get; }

    public bool IsAlphaTested => Blend == BlendAlphaTest;

    public bool IsBlended => Type == TypeTransparent || Blend >= BlendAlpha;

    public bool IsTwoSided => (Flags & FlagTwoSided) != 0;

    public bool WritesDepth => (Flags & FlagNoDepthWrite) == 0;

    public bool DepthTestAlways => (Flags & FlagDepthTestAlways) != 0;

    public bool IsDepthOnly => (Flags & FlagDepthOnly) != 0;

    public bool IsDrawn => (Flags & (FlagDepthOnly | FlagNoDepthWrite)) !=
                           (FlagDepthOnly | FlagNoDepthWrite);
}

public sealed class M3Texture
{
    public const int NoSlot = 0xFFFF;
    public const int MaxSlot = 13;

    public const int FallbackColour = 0;
    public const int FallbackNormal = 1;
    public const int FallbackSpecial = 2;

    public M3Texture(int slot, int fallbackKind, uint usageClass, float param08, string path)
    {
        Slot = slot;
        FallbackKind = fallbackKind;
        UsageClass = usageClass;
        Param08 = param08;
        Path = path;
    }

    public int Slot { get; }

    public int FallbackKind { get; }

    public uint UsageClass { get; }

    public float Param08 { get; }

    public string Path { get; }

    public bool IsSubstitutable => Slot != NoSlot && Slot < MaxSlot;
}
