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

    [Export] public NodePath PlayerPath { get; set; } = new NodePath("../AnimationPlayer");

    private M3File? _model;
    private M3PoseRuntime? _runtime;
    private Skeleton3D? _skeleton;
    private AnimationPlayer? _player;

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
        _player = GetNodeOrNull<AnimationPlayer>(PlayerPath);

        if (_skeleton is null || _skeleton.GetBoneCount() != model.Bones.Length)
        {
            _skeleton = null;
            return;
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

        SelectIndex(index);
        _elapsedMs = 0.0;
        Playing = true;
    }

    private void SelectIndex(int index)
    {
        M3Animation animation = _model!.Animations[index];
        _sequence = index;
        _start = animation.Start;
        _end = animation.End;
        _loops = animation.Loops;
    }

    public override void _Process(double delta)
    {
        if (_runtime is null || _skeleton is null || _sequence < 0)
        {
            return;
        }

        if (ApplyPlayerPose())
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

    private bool ApplyPlayerPose()
    {
        if (_player is null || _model is null || _runtime is null || _skeleton is null)
        {
            return false;
        }

        StringName current = _player.CurrentAnimation;
        if (current.IsEmpty || !_player.HasAnimation(current))
        {
            return false;
        }

        Animation clip = _player.GetAnimation(current);
        if (!clip.HasMeta(M3AnimationBaker.SourceIndexMeta))
        {
            return false;
        }

        int index = clip.GetMeta(M3AnimationBaker.SourceIndexMeta).AsInt32();
        if (index < 0 || index >= _model.Animations.Length)
        {
            return false;
        }

        if (_sequence != index)
        {
            SelectIndex(index);
        }

        uint duration = _end > _start ? _end - _start : 0;
        double length = _player.CurrentAnimationLength;
        double position = length > 0.0
            ? Math.Clamp(_player.CurrentAnimationPosition / length, 0.0, 1.0)
            : 0.0;
        _elapsedMs = position * duration;
        ApplyPose(_start + (uint)_elapsedMs);
        return true;
    }

    public float PoseRoundTripError()
    {
        if (_runtime is null || _skeleton is null)
        {
            return -1.0f;
        }

        float worst = 0.0f;

        for (int i = 0; i < _runtime.BoneCount; i++)
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

        for (int i = 0; i < _runtime.BoneCount; i++)
        {
            ReadOnlySpan<float> translation = _runtime.WorldTranslation(i);
            ReadOnlySpan<float> rotation = _runtime.WorldRotation(i);
            ReadOnlySpan<float> scale = _runtime.WorldScale(i);

            _skeleton.SetBonePosePosition(
                i, new Vector3(translation[0], translation[1], translation[2]));
            _skeleton.SetBonePoseRotation(
                i, new Quaternion(rotation[0], rotation[1], rotation[2], rotation[3]));
            _skeleton.SetBonePoseScale(i, new Vector3(scale[0], scale[1], scale[2]));
        }
    }
}
