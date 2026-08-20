using System;

namespace WildStar.Model;

public sealed class M3PoseRuntime
{
    public const int MaxLayers = 16;

    public struct Layer
    {
        public uint Time;
        public float Weight;
    }

    private readonly M3File _model;
    private readonly float[] _scale;
    private readonly float[] _rotation;
    private readonly float[] _translation;
    private readonly float[] _matrix;

    private readonly float[] _restScale;
    private readonly float[] _restRotation;
    private readonly float[] _restTranslation;

    private readonly Layer[] _layers = new Layer[MaxLayers];
    private int _layerCount;

    public M3PoseRuntime(M3File model)
    {
        _model = model;
        int n = model.Bones.Length;
        _scale = new float[n * 3];
        _rotation = new float[n * 4];
        _translation = new float[n * 3];
        _matrix = new float[n * 16];

        _restScale = new float[n * 3];
        _restRotation = new float[n * 4];
        _restTranslation = new float[n * 3];
        BuildRestLocals();
    }

    private void BuildRestLocals()
    {
        Span<float> scale = stackalloc float[3];
        Span<float> rotation = stackalloc float[4];
        Span<float> translation = stackalloc float[3];

        for (int i = 0; i < _model.Bones.Length; i++)
        {
            M3Bone bone = _model.Bones[i];
            float[] local;

            if (bone.IsRoot || bone.Parent >= _model.Bones.Length || bone.Parent == i)
            {
                local = bone.Bind;
            }
            else
            {
                float[]? parentInverse = M3Pose.AffineInverse(_model.Bones[bone.Parent].Bind);
                local = parentInverse is null
                    ? bone.Bind
                    : M3Pose.Multiply(parentInverse, bone.Bind);
            }

            M3Pose.Decompose(local, scale, rotation, translation);

            scale.CopyTo(_restScale.AsSpan(i * 3, 3));
            rotation.CopyTo(_restRotation.AsSpan(i * 4, 4));
            translation.CopyTo(_restTranslation.AsSpan(i * 3, 3));
        }
    }

    public int BoneCount => _model.Bones.Length;

    public ReadOnlySpan<float> World(int bone) => _matrix.AsSpan(bone * 16, 16);

    public void SetSingle(uint time)
    {
        _layers[0].Time = time;
        _layers[0].Weight = 1.0f;
        _layerCount = 1;
    }

    public void SetLayers(ReadOnlySpan<Layer> layers)
    {
        _layerCount = Math.Min(layers.Length, MaxLayers);
        for (int i = 0; i < _layerCount; i++)
        {
            _layers[i] = layers[i];
        }
    }

    public void Evaluate()
    {
        Span<float> sampled = stackalloc float[4];
        Span<float> layerValue = stackalloc float[4];
        Span<float> layerDivisor = stackalloc float[4];
        Span<float> accum = stackalloc float[4];
        Span<float> identity = stackalloc float[4] { 0.0f, 0.0f, 0.0f, 1.0f };
        Span<float> blended = stackalloc float[4];
        Span<float> point = stackalloc float[3];

        for (int i = 0; i < _model.Bones.Length; i++)
        {
            M3Bone bone = _model.Bones[i];

            int s = i * 3, q = i * 4, t = i * 3;

            int parent = -1;
            if (!bone.IsRoot && bone.Parent < _model.Bones.Length && bone.Parent != i &&
                (~bone.Flags & M3Bone.NoInheritAnyFlag) != 0)
            {
                parent = bone.Parent;
            }

            int ps = parent * 3, pq = parent * 4;

            if (bone.Scale.HasKeys)
            {
                SampleVec3Layered(bone.Scale, sampled);
                _scale[s] = sampled[0];
                _scale[s + 1] = sampled[1];
                _scale[s + 2] = sampled[2];

                if (parent >= 0 && bone.InheritsScale)
                {
                    _scale[s] *= _scale[ps];
                    _scale[s + 1] *= _scale[ps + 1];
                    _scale[s + 2] *= _scale[ps + 2];
                }
            }
            else if (parent < 0 || !bone.InheritsScale)
            {
                _scale[s] = _scale[s + 1] = _scale[s + 2] = 1.0f;
            }
            else
            {
                _scale[s] = _scale[ps];
                _scale[s + 1] = _scale[ps + 1];
                _scale[s + 2] = _scale[ps + 2];
            }

            if (bone.ScaleDivisor.HasKeys)
            {
                SampleVec3Layered(bone.ScaleDivisor, sampled);
                for (int c = 0; c < 3; c++)
                {
                    if (MathF.Abs(sampled[c]) > M3File.DivisorEpsilon)
                    {
                        _scale[s + c] /= sampled[c];
                    }
                }
            }

            if (bone.ScaleLayer.HasKeys && _layerCount > 0)
            {
                for (int layer = _layerCount; layer > 0; layer--)
                {
                    uint time = _layers[layer - 1].Time;
                    bone.ScaleLayer.Sample(time, layerValue);

                    if (bone.ScaleDivisorLayer.HasKeys)
                    {
                        bone.ScaleDivisorLayer.Sample(time, layerDivisor);
                        for (int c = 0; c < 3; c++)
                        {
                            if (MathF.Abs(layerDivisor[c]) > M3File.DivisorEpsilon)
                            {
                                layerValue[c] /= layerDivisor[c];
                            }
                        }
                    }

                    float w = _layers[layer - 1].Weight;
                    _scale[s] *= (layerValue[0] - 1.0f) * w + 1.0f;
                    _scale[s + 1] *= (layerValue[1] - 1.0f) * w + 1.0f;
                    _scale[s + 2] *= (layerValue[2] - 1.0f) * w + 1.0f;
                }
            }

            if (bone.Rotation.HasKeys)
            {
                SampleQuatLayered(bone.Rotation, sampled);
                _rotation[q] = sampled[0];
                _rotation[q + 1] = sampled[1];
                _rotation[q + 2] = sampled[2];
                _rotation[q + 3] = sampled[3];

                if (parent >= 0 && bone.InheritsRotation)
                {
                    M3Pose.QuaternionMultiply(
                        _rotation.AsSpan(pq, 4), _rotation.AsSpan(q, 4), sampled);
                    sampled[..4].CopyTo(_rotation.AsSpan(q, 4));
                }
            }
            else if (parent < 0 || !bone.InheritsRotation)
            {
                _rotation[q] = _rotation[q + 1] = _rotation[q + 2] = 0.0f;
                _rotation[q + 3] = 1.0f;
            }
            else
            {
                _rotation.AsSpan(pq, 4).CopyTo(_rotation.AsSpan(q, 4));
            }

            if (bone.RotationLayer.HasKeys && _layerCount > 0)
            {
                accum[0] = accum[1] = accum[2] = accum[3] = 0.0f;
                float totalWeight = 0.0f;

                for (int layer = _layerCount; layer > 0; layer--)
                {
                    bone.RotationLayer.SampleSlerp(_layers[layer - 1].Time, sampled);

                    float dot = accum[0] * sampled[0] + accum[1] * sampled[1] +
                                accum[2] * sampled[2] + accum[3] * sampled[3];
                    if (dot < 0.0f)
                    {
                        sampled[0] = -sampled[0];
                        sampled[1] = -sampled[1];
                        sampled[2] = -sampled[2];
                        sampled[3] = -sampled[3];
                    }

                    float w = _layers[layer - 1].Weight;
                    totalWeight += w;
                    accum[0] += sampled[0] * w;
                    accum[1] += sampled[1] * w;
                    accum[2] += sampled[2] * w;
                    accum[3] += sampled[3] * w;
                }

                if (totalWeight > M3File.DivisorEpsilon)
                {
                    float len2 = accum[0] * accum[0] + accum[1] * accum[1] +
                                 accum[2] * accum[2] + accum[3] * accum[3];
                    if (len2 > M3File.DivisorEpsilon)
                    {
                        float invLen = 1.0f / MathF.Sqrt(len2);
                        accum[0] *= invLen;
                        accum[1] *= invLen;
                        accum[2] *= invLen;
                        accum[3] *= invLen;

                        M3Slerp.Slerp(blended, identity, accum, totalWeight);
                        M3Pose.QuaternionMultiply(blended, _rotation.AsSpan(q, 4), sampled);
                        sampled[..4].CopyTo(_rotation.AsSpan(q, 4));
                    }
                }
            }

            if (bone.Translation.HasKeys)
            {
                SampleVec3Layered(bone.Translation, sampled);
                _translation[t] = sampled[0];
                _translation[t + 1] = sampled[1];
                _translation[t + 2] = sampled[2];
            }
            else
            {
                _translation[t] = _translation[t + 1] = _translation[t + 2] = 0.0f;
            }

            if (bone.TranslationLayer.HasKeys && _layerCount > 0)
            {
                for (int layer = _layerCount; layer > 0; layer--)
                {
                    bone.TranslationLayer.Sample(_layers[layer - 1].Time, layerValue);
                    float w = _layers[layer - 1].Weight;
                    _translation[t] += layerValue[0] * w;
                    _translation[t + 1] += layerValue[1] * w;
                    _translation[t + 2] += layerValue[2] * w;
                }
            }

            if (parent >= 0 && bone.InheritsTranslation)
            {
                M3Pose.TransformPointRowMajor(
                    _translation.AsSpan(t, 3), _matrix.AsSpan(parent * 16, 16), point);
                _translation[t] = point[0];
                _translation[t + 1] = point[1];
                _translation[t + 2] = point[2];
            }

            float[] world = M3Pose.BuildPerFrameMatrix(
                _scale.AsSpan(s, 3), _rotation.AsSpan(q, 4), _translation.AsSpan(t, 3));
            world.CopyTo(_matrix, i * 16);
        }
    }

    private void SampleVec3Layered(M3Track track, Span<float> outValue)
    {
        int last = _layerCount - 1;
        track.Sample(_layers[last].Time, outValue);

        Span<float> prev = stackalloc float[4];
        for (int i = last; i > 0; i--)
        {
            track.Sample(_layers[i - 1].Time, prev);
            float w = 1.0f - _layers[i].Weight;
            outValue[0] = (outValue[0] - prev[0]) * w + prev[0];
            outValue[1] = (outValue[1] - prev[1]) * w + prev[1];
            outValue[2] = (outValue[2] - prev[2]) * w + prev[2];
        }
    }

    private void SampleQuatLayered(M3Track track, Span<float> outValue)
    {
        int last = _layerCount - 1;
        track.SampleSlerp(_layers[last].Time, outValue);

        Span<float> prev = stackalloc float[4];
        Span<float> result = stackalloc float[4];
        for (int i = last; i > 0; i--)
        {
            track.SampleSlerp(_layers[i - 1].Time, prev);
            float w = 1.0f - _layers[i].Weight;
            M3Slerp.Slerp(result, prev, outValue, w);
            result.CopyTo(outValue);
        }
    }
}
