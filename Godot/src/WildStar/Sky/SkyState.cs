using System;
using System.Collections.Generic;

namespace WildStar.Sky;

public sealed class SkyState
{
    public const int BandCount = SkyGradient.BandCount;
    public const int ShCount = SkyLightBlock.ShCoefficients;
    public const int PostCount = SkyPostProcess.FloatCount;

    public float[] Sh { get; } = new float[ShCount];

    public float[] SkySh { get; } = new float[ShCount];

    public float[] SkyBands { get; } = new float[BandCount * 4];

    public float SkyGradientWeight { get; private set; }

    public float[] FogSh { get; } = new float[ShCount];

    public float[] FogBands { get; } = new float[BandCount * 4];

    public float FogGradientWeight { get; private set; }

    public float[] Fog { get; } = new float[6];

    public float FogStart => Fog[0];

    public float FogEnd => Fog[1];

    public float FogDensity => Fog[4];

    public float[] SunDirection { get; } = new float[3];

    public float[] SunColour { get; } = new float[3];

    public bool HasSun { get; private set; }

    public float[] Post { get; } = new float[PostCount];

    public List<(string Path, float Weight)> Luts { get; } = new();

    public float DomeYaw { get; private set; }

    public float DomePitch { get; private set; }

    public static readonly float[] PostDefaults = BuildPostDefaults();

    private static float[] BuildPostDefaults()
    {
        var p = new float[PostCount];
        p[0] = 1.0f; p[1] = 1.0f; p[2] = 1.0f; p[3] = 1.0f;
        p[6] = 1.0f; p[7] = 1.0f; p[8] = 1.0f; p[9] = 1.0f;
        p[15] = 1.3f; p[16] = 1.0f; p[17] = 1.0f;
        return p;
    }

    public static SkyState Sample(SkyFile sky, uint time)
    {
        var state = new SkyState();
        state.Accumulate(sky, time, 1.0f);
        state.Finish();
        return state;
    }

    public static SkyState Blend(IReadOnlyList<(SkyFile Sky, float Weight)> sources, uint time)
    {
        var state = new SkyState();
        foreach ((SkyFile sky, float weight) in sources)
        {
            if (weight > 0.0f)
            {
                state.Accumulate(sky, time, weight);
            }
        }

        state.Finish();
        return state;
    }

    private float _sunWeight;
    private float _postWeight;

    private void Accumulate(SkyFile sky, uint time, float weight)
    {
        float[] sh = SkyLighting.AccumulateBlock(sky.Blocks[SkyFile.LightingBlockIndex], time);
        SkyColour night = SkyColour.Sample(sky.NightAmbient, time, 0, SkyColour.Zero);
        sh[0] += night.R * SkyLighting.NightScaleDefault * SkyLighting.TwoSqrtPi;
        sh[9] += night.G * SkyLighting.NightScaleDefault * SkyLighting.TwoSqrtPi;
        sh[18] += night.B * SkyLighting.NightScaleDefault * SkyLighting.TwoSqrtPi;
        for (int i = 0; i < ShCount; i++)
        {
            Sh[i] += sh[i] * weight;
        }

        SkyLightBlock skyBlock = sky.Blocks[SkyFile.SkyBlockIndex];
        float[] skySh = SkyLighting.AccumulateBlock(skyBlock, time);
        for (int i = 0; i < ShCount; i++)
        {
            SkySh[i] += skySh[i] * weight;
        }

        if (skyBlock.Gradient.HasKeys)
        {
            SkyGradient gradient = SkyGradient.Sample(skyBlock.Gradient, time);
            SkyGradientWeight += weight;
            DomeYaw += gradient.Value0 * weight;
            DomePitch += gradient.Value1 * weight;
            AddBands(SkyBands, gradient, weight);
        }

        SkyLightBlock fogBlock = sky.Blocks[SkyFile.FogBlockIndex];
        float[] fogSh = SkyLighting.AccumulateBlock(fogBlock, time);
        for (int i = 0; i < ShCount; i++)
        {
            FogSh[i] += fogSh[i] * weight;
        }

        if (fogBlock.Gradient.HasKeys)
        {
            FogGradientWeight += weight;
            AddBands(FogBands, SkyGradient.Sample(fogBlock.Gradient, time), weight);
        }

        SkyFog fog = SkyFog.Sample(sky.Fog, time);
        for (int i = 0; i < 6; i++)
        {
            Fog[i] += fog.Values[i] * weight;
        }

        if (sky.Sun.HasKeys)
        {
            SkyDirectionalLight light = SkyDirectionalLight.Sample(sky.Sun, time);
            float[] d = SkyLighting.SunDirection(light.Direction);
            _sunWeight += weight;
            for (int i = 0; i < 3; i++)
            {
                SunDirection[i] += d[i] * weight;
            }

            SunColour[0] += light.Colour.R * weight;
            SunColour[1] += light.Colour.G * weight;
            SunColour[2] += light.Colour.B * weight;
        }

        if (sky.PostProcess.HasKeys)
        {
            SkyPostProcess post = SkyPostProcess.Sample(sky.PostProcess, time);
            for (int i = 0; i < PostCount; i++)
            {
                Post[i] += post.Values[i] * weight;
            }
        }
        else
        {
            for (int i = 0; i < PostCount; i++)
            {
                Post[i] += PostDefaults[i] * weight;
            }
        }

        _postWeight += weight;

        if (sky.ColourLut.Length > 0)
        {
            Luts.Add((sky.ColourLut, weight));
        }
    }

    private static void AddBands(float[] target, SkyGradient gradient, float weight)
    {
        for (int i = 0; i < BandCount; i++)
        {
            SkyColour band = gradient.Bands[i];
            target[4 * i] += band.R * weight;
            target[4 * i + 1] += band.G * weight;
            target[4 * i + 2] += band.B * weight;
            target[4 * i + 3] += band.A * weight;
        }
    }

    private void Finish()
    {
        if (_sunWeight > 0.0f)
        {
            HasSun = true;
            float length = MathF.Sqrt(SunDirection[0] * SunDirection[0] + SunDirection[1] * SunDirection[1] +
                                      SunDirection[2] * SunDirection[2]);
            if (length > 0.000001f)
            {
                for (int i = 0; i < 3; i++)
                {
                    SunDirection[i] /= length;
                }
            }

            for (int i = 0; i < 3; i++)
            {
                SunColour[i] *= SkyLighting.LightingColourScale;
            }
        }

        if (_postWeight <= 0.0f)
        {
            Array.Copy(PostDefaults, Post, PostCount);
        }
    }

    public bool HasColourGrading
    {
        get
        {
            if (Luts.Count > 0)
            {
                return true;
            }

            for (int i = 0; i < PostCount; i++)
            {
                if (MathF.Abs(Post[i] - PostDefaults[i]) > 0.0005f && (i <= 3 || (i >= 6 && i <= 9)))
                {
                    return true;
                }
            }

            return false;
        }
    }

    public static readonly float[] BandElevations =
    {
        90.0f, 82.5f, 75.0f, 67.5f, 60.0f, 52.5f, 45.0f, 37.5f, 30.0f, 22.5f, 15.0f, 7.5f, 0.0f, -30.0f, -60.0f, -90.0f,
    };
}
