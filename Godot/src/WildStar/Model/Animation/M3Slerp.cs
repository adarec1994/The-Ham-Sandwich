using System;

namespace WildStar.Model;

public static class M3Slerp
{
    private const float SlerpEpsilon = 0.0000099999997f;
    private const float PiOver2 = 1.5707963f;

    public static void Slerp(Span<float> output, ReadOnlySpan<float> a, ReadOnlySpan<float> b, float t)
    {
        float w0 = 1.0f - t;
        float w1 = t;

        float dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
        float absDot = MathF.Abs(dot);

        if (1.0f - absDot > SlerpEpsilon)
        {
            float sq = 1.0f - absDot * absDot;
            float invSqrt = 1.0f / MathF.Sqrt(sq);
            float refined = (invSqrt * sq * invSqrt - 3.0f) * (invSqrt * -0.5f);
            float sinAngle = refined * sq;

            float ratio;
            float @base;
            if (sinAngle <= absDot)
            {
                @base = 0.0f;
                ratio = sinAngle / absDot;
            }
            else
            {
                @base = PiOver2;
                ratio = -(absDot / sinAngle);
            }

            float r2 = ratio * ratio;
            float angle = ((((((((r2 * 0.0028662258f - 0.016165737f) * r2
                                + 0.042909615f) * r2
                               - 0.075289637f) * r2
                              + 0.10656264f) * r2
                             - 0.14208899f) * r2
                            + 0.19993551f) * r2
                           - 0.33333147f) * r2
                          + 1.0f) * ratio + @base;

            float angle0 = angle * w0;
            float angle1 = angle * w1;

            w0 = MathF.Sin(angle0) * refined;
            w1 = MathF.Sin(angle1) * refined;
        }

        if (dot < 0.0f)
        {
            w1 = -w1;
        }

        output[0] = w1 * b[0] + w0 * a[0];
        output[1] = w1 * b[1] + w0 * a[1];
        output[2] = w1 * b[2] + w0 * a[2];
        output[3] = w1 * b[3] + w0 * a[3];
    }
}
