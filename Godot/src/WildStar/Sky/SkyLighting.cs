using System;

namespace WildStar.Sky;

public static class SkyLighting
{
    public const float DegreesToRadians = 0.017453292f;
    public const float TwoSqrtPi = 3.5449078f;
    public const float Y00 = 0.28209481f;
    public const float Y1 = 0.48860252f;
    public const float Y2a = 1.0925485f;
    public const float Y20 = 0.31539157f;
    public const float Y22 = 0.54627424f;
    public const float ConeAngleEpsilon = 0.000099999997f;
    public const float NightScaleDefault = 1.0f;
    public const float LightingColourScale = 3.0f;

    public static float[] SunDirection(SkyDirection direction)
    {
        float a = direction.Elevation * DegreesToRadians;
        float b = direction.Azimuth * DegreesToRadians;
        float cosA = MathF.Cos(a);
        return new[] { -(MathF.Sin(b) * cosA), MathF.Sin(a), -(MathF.Cos(b) * cosA) };
    }

    public static float[] LightDirection(SkyDirection direction)
    {
        float a = direction.Elevation * DegreesToRadians;
        float b = direction.Azimuth * DegreesToRadians;
        float cosA = MathF.Cos(a);
        return new[] { MathF.Cos(b) * cosA, MathF.Sin(a), MathF.Sin(b) * cosA };
    }

    public static float[] ModelRotation(SkyDirection direction)
    {
        float a = direction.Elevation * DegreesToRadians;
        float b = direction.Azimuth * DegreesToRadians;
        float sinA = MathF.Sin(a);
        float cosA = MathF.Cos(a);
        float sinB = MathF.Sin(b);
        float cosB = MathF.Cos(b);
        return new[]
        {
            cosB, 0.0f, -sinB,
            sinB * sinA, cosA, cosB * sinA,
            sinB * cosA, -sinA, cosB * cosA,
        };
    }

    public static float[] AccumulateBlock(SkyLightBlock block, uint time)
    {
        var sh = new float[SkyLightBlock.ShCoefficients];
        AccumulateBlock(block, time, sh);
        return sh;
    }

    public static void AccumulateBlock(SkyLightBlock block, uint time, float[] sh)
    {
        foreach (SkyTrack track in block.AmbientLights)
        {
            SkyColour c = SkyColour.Sample(track, time, 0, SkyColour.Zero);
            sh[0] += c.R * TwoSqrtPi;
            sh[9] += c.G * TwoSqrtPi;
            sh[18] += c.B * TwoSqrtPi;
        }

        foreach (SkyTrack track in block.DirectionalLights)
        {
            SkyDirectionalLight light = SkyDirectionalLight.Sample(track, time);
            AddDirectional(sh, light.Colour, LightDirection(light.Direction));
        }

        foreach (SkyTrack track in block.HemisphereLights)
        {
            SkyHemisphereLight light = SkyHemisphereLight.Sample(track, time);
            AddHemisphere(sh, light.SkyColour, light.GroundColour, LightDirection(light.Direction));
        }

        foreach (SkyTrack track in block.ShLights)
        {
            float[] values = track.SampleFloats(time, 0, SkyLightBlock.ShCoefficients, null);
            for (int i = 0; i < SkyLightBlock.ShCoefficients; i++)
            {
                sh[i] += values[i];
            }
        }

        foreach (SkyTrack track in block.ConeLights)
        {
            SkyConeLight light = SkyConeLight.Sample(track, time);
            AddCone(sh, light.Colour, LightDirection(light.Direction), light.Angle);
        }

        foreach (SkyTrack track in block.SphereLights)
        {
            SkySphereLight light = SkySphereLight.Sample(track, time);
            float[] d = LightDirection(light.Direction);
            AddSphere(sh, light.Colour,
                      new[] { d[0] * light.Distance, d[1] * light.Distance, d[2] * light.Distance },
                      light.Radius);
        }

        if (block.BaseSh.HasKeys)
        {
            float[] values = block.BaseSh.SampleFloats(time, 0, SkyLightBlock.ShCoefficients, null);
            for (int i = 0; i < SkyLightBlock.ShCoefficients; i++)
            {
                sh[i] += values[i];
            }
        }
    }

    public static void AddDirectional(float[] sh, SkyColour colour, float[] d)
    {
        float x = d[0];
        float y = d[1];
        float z = d[2];
        float[] basis =
        {
            Y00,
            -Y1 * y,
            Y1 * z,
            -Y1 * x,
            Y2a * x * y,
            -Y2a * z * y,
            Y20 * (3.0f * z * z - 1.0f),
            -Y2a * x * z,
            Y22 * (x * x - y * y),
        };

        for (int i = 0; i < 9; i++)
        {
            sh[i] += basis[i] * colour.R;
            sh[9 + i] += basis[i] * colour.G;
            sh[18 + i] += basis[i] * colour.B;
        }
    }

    public static void AddHemisphere(float[] sh, SkyColour sky, SkyColour ground, float[] d)
    {
        float[] average = { (sky.R + ground.R) * 0.5f, (sky.G + ground.G) * 0.5f, (sky.B + ground.B) * 0.5f };
        float[] delta = { (sky.R - average[0]) * 3.0699801f, (sky.G - average[1]) * 3.0699801f, (sky.B - average[2]) * 3.0699801f };
        for (int c = 0; c < 3; c++)
        {
            int o = 9 * c;
            sh[o] += average[c] * 5.3173614f;
            sh[o + 1] += delta[c] * -d[1];
            sh[o + 2] += delta[c] * d[2];
            sh[o + 3] += delta[c] * -d[0];
        }
    }

    public static void AddCone(float[] sh, SkyColour colour, float[] d, float angle)
    {
        if (angle < ConeAngleEpsilon)
        {
            AddDirectional(sh, colour, d);
            return;
        }

        float x = d[0];
        float y = d[1];
        float z = d[2];
        float cosAngle = MathF.Cos(angle);
        float sinAngle = MathF.Sin(angle);
        float c0 = 1.7724539f - cosAngle * 1.7724539f;
        float c1 = sinAngle * sinAngle * 3.0699801f * 0.5f;
        float c2 = (cosAngle * cosAngle - 1.0f) * (cosAngle * -3.9633274f) * 0.5f;
        float q0 = x * 8.6602545f * y * 0.2f;
        float q1 = y * 8.6602545f * z * -0.2f;
        float q2 = z * z - x * x * 0.5f - y * y * 0.5f;
        float q3 = x * 8.6602545f * z * -0.2f;
        float q4 = x * x * 8.6602545f * 0.1f - y * y * 8.6602545f * 0.1f;
        float[] rgb = { colour.R, colour.G, colour.B };
        for (int c = 0; c < 3; c++)
        {
            int o = 9 * c;
            float v = rgb[c];
            sh[o] += c0 * v;
            sh[o + 1] += c1 * v * -y;
            sh[o + 2] += c1 * v * z;
            sh[o + 3] += c1 * v * -x;
            sh[o + 4] += c2 * v * q0;
            sh[o + 5] += c2 * v * q1;
            sh[o + 6] += c2 * v * q2;
            sh[o + 7] += c2 * v * q3;
            sh[o + 8] += c2 * v * q4;
        }
    }

    public static void AddSphere(float[] sh, SkyColour colour, float[] position, float radius)
    {
        float length = MathF.Sqrt(position[0] * position[0] + position[1] * position[1] + position[2] * position[2]);
        float[] d = length > 0.0f
            ? new[] { position[0] / length, position[1] / length, position[2] / length }
            : new[] { 0.0f, 0.0f, 0.0f };
        float angle = length <= radius ? MathF.PI * 0.5f : MathF.Asin(radius / length);
        AddCone(sh, colour, d, angle);
    }

    public static float[] Irradiance(float[] sh)
    {
        return new[] { sh[0] * Y00, sh[9] * Y00, sh[18] * Y00 };
    }

    public static float[] Evaluate(float[] sh, float[] n)
    {
        float x = n[0];
        float y = n[1];
        float z = n[2];
        float[] basis =
        {
            Y00,
            -Y1 * y,
            Y1 * z,
            -Y1 * x,
            Y2a * x * y,
            -Y2a * z * y,
            Y20 * (3.0f * z * z - 1.0f),
            -Y2a * x * z,
            Y22 * (x * x - y * y),
        };

        var result = new float[3];
        for (int c = 0; c < 3; c++)
        {
            float sum = 0.0f;
            for (int i = 0; i < 9; i++)
            {
                sum += sh[9 * c + i] * basis[i];
            }

            result[c] = sum;
        }

        return result;
    }
}
