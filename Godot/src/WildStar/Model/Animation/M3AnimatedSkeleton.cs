using System;
using Godot;

namespace WildStar.Model;

[Tool]
[GlobalClass]
public partial class M3AnimatedSkeleton : Node
{
    [Export] public byte[] ModelData { get; set; } = Array.Empty<byte>();

    [Export] public NodePath SkeletonPath { get; set; } = new NodePath("..");

    [Export] public float TimeScale { get; set; } = 1.0f;

    [Export] public bool Playing { get; set; } = true;

    private M3File? _model;
    private M3PoseRuntime? _runtime;
    private Skeleton3D? _skeleton;
    private int[] _parent = Array.Empty<int>();

    private int _sequence = -1;
    private uint _start;
    private uint _end;
    private bool _loops = true;
    private double _elapsedMs;

    public override void _Ready()
    {
        if (ModelData.Length == 0 || !M3File.TryParse(ModelData, out M3File model, out _))
        {
            return;
        }

        _model = model;
        _runtime = new M3PoseRuntime(model);
        _skeleton = GetNodeOrNull<Skeleton3D>(SkeletonPath);

        if (_skeleton is null || _skeleton.GetBoneCount() != model.Bones.Length)
        {
            _skeleton = null;
            return;
        }

        _parent = new int[model.Bones.Length];
        for (int i = 0; i < model.Bones.Length; i++)
        {
            M3Bone bone = model.Bones[i];
            _parent[i] = !bone.IsRoot && bone.Parent < model.Bones.Length && bone.Parent != i
                ? bone.Parent
                : -1;
        }

        if (model.Animations.Length > 0)
        {
            PlayIndex(0);
        }
    }

    public bool Play(int sequenceId, int variation)
    {
        if (_model is null)
        {
            return false;
        }

        for (int i = 0; i < _model.Animations.Length; i++)
        {
            M3Animation a = _model.Animations[i];
            if (a.SequenceId == sequenceId && a.Variation == variation)
            {
                PlayIndex(i);
                return true;
            }
        }

        return false;
    }

    public void Stop() => Playing = false;

    public void SeekNormalized(double position)
    {
        if (_sequence < 0)
        {
            return;
        }

        uint duration = _end > _start ? _end - _start : 0;
        _elapsedMs = duration > 0 ? Math.Clamp(position, 0.0, 1.0) * duration : 0.0;
        ApplyPose(_start + (uint)_elapsedMs);
    }

    public void PlayIndex(int index)
    {
        if (_model is null || index < 0 || index >= _model.Animations.Length)
        {
            return;
        }

        M3Animation animation = _model.Animations[index];
        _sequence = index;
        _start = animation.Start;
        _end = animation.End;
        _loops = animation.Loops;
        _elapsedMs = 0.0;
        Playing = true;
    }

    public override void _Process(double delta)
    {
        if (_runtime is null || _skeleton is null || _sequence < 0)
        {
            return;
        }

        uint duration = _end > _start ? _end - _start : 0;

        if (Playing && duration > 0)
        {
            _elapsedMs += delta * 1000.0 * TimeScale;

            if (_loops)
            {
                _elapsedMs %= duration;
                if (_elapsedMs < 0.0)
                {
                    _elapsedMs += duration;
                }
            }
            else if (_elapsedMs >= duration)
            {
                _elapsedMs = duration;
                Playing = false;
            }
            else if (_elapsedMs < 0.0)
            {
                _elapsedMs = 0.0;
            }
        }

        ApplyPose(_start + (uint)_elapsedMs);
    }

    public float PoseRoundTripError()
    {
        if (_runtime is null || _skeleton is null)
        {
            return -1.0f;
        }

        float worst = 0.0f;

        for (int i = 0; i < _parent.Length; i++)
        {
            Transform3D want = M3Matrix.ToTransform(_runtime.World(i));
            Transform3D got = _skeleton.GetBoneGlobalPose(i);

            for (int c = 0; c < 3; c++)
            {
                worst = Mathf.Max(worst, (want.Basis[c] - got.Basis[c]).Length());
            }

            worst = Mathf.Max(worst, (want.Origin - got.Origin).Length());
        }

        return worst;
    }

    private void ApplyPose(uint time)
    {
        if (_runtime is null || _skeleton is null)
        {
            return;
        }

        _runtime.SetSingle(time);
        _runtime.Evaluate();

        for (int i = 0; i < _parent.Length; i++)
        {
            Transform3D global = M3Matrix.ToTransform(_runtime.World(i));

            int p = _parent[i];
            Transform3D local = p >= 0
                ? M3Matrix.ToTransform(_runtime.World(p)).AffineInverse() * global
                : global;

            _skeleton.SetBonePosePosition(i, local.Origin);
            _skeleton.SetBonePoseRotation(i, local.Basis.GetRotationQuaternion());
            _skeleton.SetBonePoseScale(i, local.Basis.Scale);
        }
    }
}
