using System;

namespace WildStar.Model;

public static class M3Pose
{
    public const float RotationScale = 1.0f / 16384.0f;

    public static void QuaternionMultiply(ReadOnlySpan<float> p, ReadOnlySpan<float> c,
                                          Span<float> outQ)
    {
        float px = p[0], py = p[1], pz = p[2], pw = p[3];
        float cx = c[0], cy = c[1], cz = c[2], cw = c[3];

        float x = px * cw + cx * pw + cz * py - cy * pz;
        float y = cy * pw - cz * px + py * cw + pz * cx;
        float z = cy * px + cz * pw - py * cx + pz * cw;
        float w = cw * pw - px * cx - cy * py - pz * cz;

        outQ[0] = x;
        outQ[1] = y;
        outQ[2] = z;
        outQ[3] = w;
    }

    public static float[] QuaternionToMatrixRowMajor(ReadOnlySpan<float> q)
    {
        float qx = q[0], qy = q[1], qz = q[2], qw = q[3];

        float twoY2 = qy * (qy * 2.0f);
        float twoXY = qx * (qy * 2.0f);
        float twoWX = qw * (qx * 2.0f);
        float twoWY = qw * (qy * 2.0f);
        float twoZ = qz * 2.0f;
        float oneMinusTwoX2 = 1.0f - qx * (qx * 2.0f);
        float twoZ2 = qz * (qz * 2.0f);
        float twoWZ = qw * twoZ;
        float twoYZ = qy * twoZ;

        var m = new float[16];

        m[0] = (1.0f - twoY2) - twoZ2;
        m[1] = twoXY + twoWZ;
        m[2] = (qx * twoZ) - twoWY;
        m[3] = 0.0f;

        m[4] = twoXY - twoWZ;
        m[5] = oneMinusTwoX2 - twoZ2;
        m[6] = twoYZ + twoWX;
        m[7] = 0.0f;

        m[8] = (qx * twoZ) + twoWY;
        m[9] = twoYZ - twoWX;
        m[10] = oneMinusTwoX2 - twoY2;
        m[11] = 0.0f;

        m[12] = 0.0f;
        m[13] = 0.0f;
        m[14] = 0.0f;
        m[15] = 1.0f;

        return m;
    }

    public static void TransformPointRowMajor(ReadOnlySpan<float> p, ReadOnlySpan<float> m,
                                              Span<float> outP)
    {
        float px = p[0], py = p[1], pz = p[2];

        float x = px * m[0] + py * m[4] + pz * m[8] + m[12];
        float y = px * m[1] + py * m[5] + pz * m[9] + m[13];
        float z = px * m[2] + py * m[6] + pz * m[10] + m[14];

        outP[0] = x;
        outP[1] = y;
        outP[2] = z;
    }

    public static float[] Multiply(ReadOnlySpan<float> a, ReadOnlySpan<float> b)
    {
        var result = new float[16];

        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                float sum = 0.0f;

                for (int k = 0; k < 4; k++)
                {
                    sum += a[i * 4 + k] * b[k * 4 + j];
                }

                result[i * 4 + j] = sum;
            }
        }

        return result;
    }

    public static float[]? AffineInverse(ReadOnlySpan<float> m)
    {
        float a = m[0], b = m[1], c = m[2];
        float d = m[4], e = m[5], f = m[6];
        float g = m[8], h = m[9], i = m[10];

        float det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
        if (MathF.Abs(det) < 1e-12f)
        {
            return null;
        }

        float inv = 1.0f / det;
        var r = new float[16];

        r[0] = (e * i - f * h) * inv;
        r[1] = (c * h - b * i) * inv;
        r[2] = (b * f - c * e) * inv;
        r[4] = (f * g - d * i) * inv;
        r[5] = (a * i - c * g) * inv;
        r[6] = (c * d - a * f) * inv;
        r[8] = (d * h - e * g) * inv;
        r[9] = (b * g - a * h) * inv;
        r[10] = (a * e - b * d) * inv;

        float tx = m[12], ty = m[13], tz = m[14];
        r[12] = -(tx * r[0] + ty * r[4] + tz * r[8]);
        r[13] = -(tx * r[1] + ty * r[5] + tz * r[9]);
        r[14] = -(tx * r[2] + ty * r[6] + tz * r[10]);
        r[15] = 1.0f;

        return r;
    }

    public static void Decompose(ReadOnlySpan<float> m, Span<float> scale, Span<float> rotation,
                                 Span<float> translation)
    {
        for (int row = 0; row < 3; row++)
        {
            int o = row * 4;
            scale[row] = MathF.Sqrt(m[o] * m[o] + m[o + 1] * m[o + 1] + m[o + 2] * m[o + 2]);
        }

        translation[0] = m[12];
        translation[1] = m[13];
        translation[2] = m[14];

        float determinant =
            m[0] * (m[5] * m[10] - m[6] * m[9]) -
            m[1] * (m[4] * m[10] - m[6] * m[8]) +
            m[2] * (m[4] * m[9] - m[5] * m[8]);

        if (determinant < 0.0f)
        {
            scale[0] = -scale[0];
        }

        Span<float> r = stackalloc float[9];
        for (int row = 0; row < 3; row++)
        {
            float s = MathF.Abs(scale[row]) > 1e-8f ? 1.0f / scale[row] : 0.0f;
            r[row * 3] = m[row * 4] * s;
            r[row * 3 + 1] = m[row * 4 + 1] * s;
            r[row * 3 + 2] = m[row * 4 + 2] * s;
        }

        float m00 = r[0], m01 = r[1], m02 = r[2];
        float m10 = r[3], m11 = r[4], m12 = r[5];
        float m20 = r[6], m21 = r[7], m22 = r[8];

        float trace = m00 + m11 + m22;

        if (trace > 0.0f)
        {
            float s = MathF.Sqrt(trace + 1.0f) * 2.0f;
            rotation[3] = 0.25f * s;
            rotation[0] = (m12 - m21) / s;
            rotation[1] = (m20 - m02) / s;
            rotation[2] = (m01 - m10) / s;
        }
        else if (m00 > m11 && m00 > m22)
        {
            float s = MathF.Sqrt(1.0f + m00 - m11 - m22) * 2.0f;
            rotation[3] = (m12 - m21) / s;
            rotation[0] = 0.25f * s;
            rotation[1] = (m10 + m01) / s;
            rotation[2] = (m20 + m02) / s;
        }
        else if (m11 > m22)
        {
            float s = MathF.Sqrt(1.0f + m11 - m00 - m22) * 2.0f;
            rotation[3] = (m20 - m02) / s;
            rotation[0] = (m10 + m01) / s;
            rotation[1] = 0.25f * s;
            rotation[2] = (m21 + m12) / s;
        }
        else
        {
            float s = MathF.Sqrt(1.0f + m22 - m00 - m11) * 2.0f;
            rotation[3] = (m01 - m10) / s;
            rotation[0] = (m20 + m02) / s;
            rotation[1] = (m21 + m12) / s;
            rotation[2] = 0.25f * s;
        }
    }

    public static float[] BuildPerFrameMatrix(ReadOnlySpan<float> scale,
                                              ReadOnlySpan<float> rotation,
                                              ReadOnlySpan<float> translation)
    {
        float[] rot = QuaternionToMatrixRowMajor(rotation);
        var m = new float[16];

        m[0] = scale[0] * rot[0];
        m[1] = scale[0] * rot[1];
        m[2] = scale[0] * rot[2];
        m[3] = 0.0f;

        m[4] = scale[1] * rot[4];
        m[5] = scale[1] * rot[5];
        m[6] = scale[1] * rot[6];
        m[7] = 0.0f;

        m[8] = scale[2] * rot[8];
        m[9] = scale[2] * rot[9];
        m[10] = scale[2] * rot[10];
        m[11] = 0.0f;

        m[12] = translation[0];
        m[13] = translation[1];
        m[14] = translation[2];
        m[15] = 1.0f;

        return m;
    }
}
