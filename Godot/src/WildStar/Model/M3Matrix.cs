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

        if (bone.IsRoot || bone.Parent >= model.Bones.Length || bone.Parent == index)
        {
            return ToTransform(bone.Bind);
        }

        Transform3D parentWorld = ToTransform(model.Bones[bone.Parent].Bind);
        Transform3D childWorld = ToTransform(bone.Bind);
        return parentWorld.AffineInverse() * childWorld;
    }
}
