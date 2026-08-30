using System;

namespace WildStar.Model;

public sealed class M3PoseRuntime
{
    public const int MaxPrimaryLayers = 4;
    public const int MaxSecondaryLayers = 8;

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

    private readonly Layer[] _primaryLayers = new Layer[MaxPrimaryLayers];
    private readonly Layer[] _secondaryLayers = new Layer[MaxSecondaryLayers];
    private int _primaryLayerCount;
    private int _secondaryLayerCount;

    public M3PoseRuntime(M3File model)
    {
        _model = model;
        int n = model.Bones.Length;
        _scale = new float[n * 3];
        _rotation = new float[n * 4];
        _translation = new float[n * 3];
        _matrix = new float[n * 16];
        EvaluateRest();
    }

    public int BoneCount => _model.Bones.Length;

    public ReadOnlySpan<float> World(int bone) => _matrix.AsSpan(bone * 16, 16);

    public ReadOnlySpan<float> WorldScale(int bone) => _scale.AsSpan(bone * 3, 3);

    public ReadOnlySpan<float> WorldRotation(int bone) => _rotation.AsSpan(bone * 4, 4);

    public ReadOnlySpan<float> WorldTranslation(int bone) => _translation.AsSpan(bone * 3, 3);

    public void SetSingle(uint time)
    {
        _primaryLayers[0].Time = time;
        _primaryLayers[0].Weight = 1.0f;
        _primaryLayerCount = 1;
        _secondaryLayerCount = 0;
    }

    public void SetLayers(ReadOnlySpan<Layer> layers)
    {
        SetLayers(layers, ReadOnlySpan<Layer>.Empty);
    }

    public void SetLayers(
        ReadOnlySpan<Layer> primaryLayers,
        ReadOnlySpan<Layer> secondaryLayers)
    {
        _primaryLayerCount = CopyLayers(primaryLayers, _primaryLayers);
        _secondaryLayerCount = CopyLayers(secondaryLayers, _secondaryLayers);
    }

    public void Evaluate()
    {
        if (_primaryLayerCount == 0)
        {
            EvaluateRest();
            return;
        }

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

            if (bone.ScaleLayer.HasKeys && _secondaryLayerCount > 0)
            {
                for (int layer = _secondaryLayerCount; layer > 0; layer--)
                {
                    uint time = _secondaryLayers[layer - 1].Time;
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

                    float w = _secondaryLayers[layer - 1].Weight;
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

            if (bone.RotationLayer.HasKeys && _secondaryLayerCount > 0)
            {
                accum[0] = accum[1] = accum[2] = accum[3] = 0.0f;
                float totalWeight = 0.0f;

                for (int layer = _secondaryLayerCount; layer > 0; layer--)
                {
                    bone.RotationLayer.SampleSlerp(_secondaryLayers[layer - 1].Time, sampled);

                    float dot = accum[0] * sampled[0] + accum[1] * sampled[1] +
                                accum[2] * sampled[2] + accum[3] * sampled[3];
                    if (dot < 0.0f)
                    {
                        sampled[0] = -sampled[0];
                        sampled[1] = -sampled[1];
                        sampled[2] = -sampled[2];
                        sampled[3] = -sampled[3];
                    }

                    float w = _secondaryLayers[layer - 1].Weight;
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

            if (bone.TranslationLayer.HasKeys && _secondaryLayerCount > 0)
            {
                for (int layer = _secondaryLayerCount; layer > 0; layer--)
                {
                    bone.TranslationLayer.Sample(_secondaryLayers[layer - 1].Time, layerValue);
                    float w = _secondaryLayers[layer - 1].Weight;
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

    public void EvaluateRest()
    {
        Span<float> localScale = stackalloc float[3];
        Span<float> localRotation = stackalloc float[4];
        Span<float> localTranslation = stackalloc float[3];
        Span<float> point = stackalloc float[3];

        for (int i = 0; i < _model.Bones.Length; i++)
        {
            M3Bone bone = _model.Bones[i];
            int s = i * 3;
            int q = i * 4;
            int t = i * 3;
            int parent = !bone.IsRoot && bone.Parent < _model.Bones.Length && bone.Parent != i
                ? bone.Parent
                : -1;

            localScale[0] = localScale[1] = localScale[2] = 1.0f;
            if (bone.Scale.HasKeys)
            {
                bone.Scale.Values.AsSpan(0, 3).CopyTo(localScale);
            }

            if (bone.ScaleDivisor.HasKeys)
            {
                for (int c = 0; c < 3; c++)
                {
                    float divisor = bone.ScaleDivisor.Values[c];
                    if (MathF.Abs(divisor) > M3File.DivisorEpsilon)
                    {
                        localScale[c] /= divisor;
                    }
                }
            }

            _scale[s] = localScale[0];
            _scale[s + 1] = localScale[1];
            _scale[s + 2] = localScale[2];
            if (parent >= 0)
            {
                int ps = parent * 3;
                _scale[s] *= _scale[ps];
                _scale[s + 1] *= _scale[ps + 1];
                _scale[s + 2] *= _scale[ps + 2];
            }

            localRotation[0] = localRotation[1] = localRotation[2] = 0.0f;
            localRotation[3] = 1.0f;
            if (bone.Rotation.HasKeys)
            {
                bone.Rotation.Values.AsSpan(0, 4).CopyTo(localRotation);
            }

            if (parent >= 0)
            {
                M3Pose.QuaternionMultiply(
                    _rotation.AsSpan(parent * 4, 4), localRotation, _rotation.AsSpan(q, 4));
            }
            else
            {
                localRotation.CopyTo(_rotation.AsSpan(q, 4));
            }

            localTranslation.Clear();
            if (bone.Translation.HasKeys)
            {
                bone.Translation.Values.AsSpan(0, 3).CopyTo(localTranslation);
            }

            if (parent >= 0)
            {
                M3Pose.TransformPointRowMajor(
                    localTranslation, _matrix.AsSpan(parent * 16, 16), point);
                point.CopyTo(_translation.AsSpan(t, 3));
            }
            else
            {
                localTranslation.CopyTo(_translation.AsSpan(t, 3));
            }

            float[] world = M3Pose.BuildPerFrameMatrix(
                _scale.AsSpan(s, 3), _rotation.AsSpan(q, 4), _translation.AsSpan(t, 3));
            world.CopyTo(_matrix, i * 16);
        }
    }

    private void SampleVec3Layered(M3Track track, Span<float> outValue)
    {
        int last = _primaryLayerCount - 1;
        track.Sample(_primaryLayers[last].Time, outValue);

        Span<float> prev = stackalloc float[4];
        for (int i = last; i > 0; i--)
        {
            track.Sample(_primaryLayers[i - 1].Time, prev);
            float w = 1.0f - _primaryLayers[i - 1].Weight;
            outValue[0] = (outValue[0] - prev[0]) * w + prev[0];
            outValue[1] = (outValue[1] - prev[1]) * w + prev[1];
            outValue[2] = (outValue[2] - prev[2]) * w + prev[2];
        }
    }

    private void SampleQuatLayered(M3Track track, Span<float> outValue)
    {
        int last = _primaryLayerCount - 1;
        track.SampleSlerp(_primaryLayers[last].Time, outValue);

        Span<float> prev = stackalloc float[4];
        Span<float> result = stackalloc float[4];
        for (int i = last; i > 0; i--)
        {
            track.SampleSlerp(_primaryLayers[i - 1].Time, prev);
            float w = 1.0f - _primaryLayers[i - 1].Weight;
            M3Slerp.Slerp(result, prev, outValue, w);
            result.CopyTo(outValue);
        }
    }

    private static int CopyLayers(ReadOnlySpan<Layer> source, Layer[] destination)
    {
        int count = Math.Min(source.Length, destination.Length);
        source[..count].CopyTo(destination);
        return count;
    }
}
