using System;
using Godot;

namespace WildStar.Model;

public static class M3Matrix
{
    public static float[] Multiply(float[] a, float[] b)
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

    public static Transform3D ToTransform(ReadOnlySpan<float> m) =>
        new(new Basis(new Vector3(m[0], m[1], m[2]),
                      new Vector3(m[4], m[5], m[6]),
                      new Vector3(m[8], m[9], m[10])),
            new Vector3(m[12], m[13], m[14]));

    public static Transform3D LocalRest(M3File model, int index)
    {
        M3Bone bone = model.Bones[index];
        Span<float> scale = stackalloc float[3] { 1.0f, 1.0f, 1.0f };
        Span<float> rotation = stackalloc float[4] { 0.0f, 0.0f, 0.0f, 1.0f };
        Span<float> translation = stackalloc float[3];

        if (bone.Scale.HasKeys)
        {
            bone.Scale.Values.AsSpan(0, 3).CopyTo(scale);
        }

        if (bone.ScaleDivisor.HasKeys)
        {
            for (int component = 0; component < 3; component++)
            {
                float divisor = bone.ScaleDivisor.Values[component];
                if (MathF.Abs(divisor) > M3File.DivisorEpsilon)
                {
                    scale[component] /= divisor;
                }
            }
        }

        if (bone.Rotation.HasKeys)
        {
            bone.Rotation.Values.AsSpan(0, 4).CopyTo(rotation);
        }

        if (bone.Translation.HasKeys)
        {
            bone.Translation.Values.AsSpan(0, 3).CopyTo(translation);
        }

        return ToTransform(M3Pose.BuildPerFrameMatrix(scale, rotation, translation));
    }
}
