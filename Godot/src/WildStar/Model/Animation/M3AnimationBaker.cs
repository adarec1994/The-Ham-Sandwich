using System;
using System.Collections.Generic;
using Godot;

namespace WildStar.Model;

public sealed class M3BakeResult
{
    public M3BakeResult(AnimationPlayer? player, int baked, int skipped, int keys)
    {
        Player = player;
        Baked = baked;
        Skipped = skipped;
        Keys = keys;
    }

    public AnimationPlayer? Player { get; }
    public int Baked { get; }
    public int Skipped { get; }
    public int Keys { get; }
}

public static class M3AnimationBaker
{
    public static float Fps = 30.0f;
    public static int KeyBudget = 300000;

    private const int MaxFrames = 1024;
    private const float PositionEpsilon = 0.00005f;
    private const float RotationEpsilon = 0.00005f;
    private const float ScaleEpsilon = 0.00005f;

    public static M3BakeResult Build(M3File model, Skeleton3D skeleton, string skeletonName,
                                     Dictionary<uint, string>? sequenceNames = null)
    {
        if (model.Animations.Length == 0 || model.Bones.Length == 0)
        {
            return new M3BakeResult(null, 0, 0, 0);
        }

        var variations = new Dictionary<int, int>();
        foreach (M3Animation animation in model.Animations)
        {
            variations.TryGetValue(animation.SequenceId, out int count);
            variations[animation.SequenceId] = count + 1;
        }

        var runtime = new M3PoseRuntime(model);
        int bones = model.Bones.Length;

        var parent = new int[bones];
        for (int i = 0; i < bones; i++)
        {
            M3Bone bone = model.Bones[i];
            parent[i] = !bone.IsRoot && bone.Parent < bones && bone.Parent != i ? bone.Parent : -1;
        }

        var library = new AnimationLibrary();
        var used = new HashSet<string>(StringComparer.Ordinal);
        int baked = 0;
        int skipped = 0;
        int keys = 0;

        for (int index = 0; index < model.Animations.Length; index++)
        {
            if (keys >= KeyBudget)
            {
                skipped++;
                continue;
            }

            Animation? clip = Bake(model, runtime, parent, skeleton, skeletonName,
                                  index, ref keys);
            if (clip is null)
            {
                skipped++;
                continue;
            }

            library.AddAnimation(
                UniqueName(model.Animations[index], index, used, sequenceNames, variations), clip);
            baked++;
        }

        if (baked == 0)
        {
            return new M3BakeResult(null, 0, skipped, 0);
        }

        var player = new AnimationPlayer
        {
            Name = "AnimationPlayer",
            RootNode = new NodePath(".."),
        };

        player.AddAnimationLibrary(string.Empty, library);
        return new M3BakeResult(player, baked, skipped, keys);
    }

    private static string UniqueName(M3Animation animation, int index, HashSet<string> used,
                                     Dictionary<uint, string>? sequenceNames,
                                     Dictionary<int, int> variations)
    {
        string name;
        if (sequenceNames is not null &&
            sequenceNames.TryGetValue((uint)animation.SequenceId, out string? described) &&
            described.Length != 0)
        {
            name = described;
        }
        else
        {
            name = "seq" + animation.SequenceId;
        }

        if (variations.TryGetValue(animation.SequenceId, out int count) && count > 1)
        {
            name += "_v" + animation.Variation;
        }

        name = Sanitise(name);

        if (used.Add(name))
        {
            return name;
        }

        string unique = name + "_" + index;
        used.Add(unique);
        return unique;
    }

    private static string Sanitise(string name)
    {
        foreach (char bad in new[] { '/', ':', ',', '[', ']', '"', '\\' })
        {
            name = name.Replace(bad, '_');
        }

        return name;
    }

    private static Animation? Bake(M3File model, M3PoseRuntime runtime, int[] parent,
                                   Skeleton3D skeleton, string skeletonName,
                                   int index, ref int keys)
    {
        M3Animation source = model.Animations[index];
        uint duration = source.Duration;
        if (duration == 0)
        {
            return null;
        }

        int bones = model.Bones.Length;
        float seconds = source.Seconds;
        int frames = Math.Clamp((int)MathF.Ceiling(seconds * Fps) + 1, 2, MaxFrames);

        var positions = new Vector3[bones * frames];
        var rotations = new Quaternion[bones * frames];
        var scales = new Vector3[bones * frames];
        var world = new Transform3D[bones];

        for (int f = 0; f < frames; f++)
        {
            double t = (double)f / (frames - 1);
            runtime.SetSingle(source.Start + (uint)(t * duration));
            runtime.Evaluate();

            for (int b = 0; b < bones; b++)
            {
                world[b] = M3Matrix.ToTransform(runtime.World(b));
            }

            for (int b = 0; b < bones; b++)
            {
                int p = parent[b];
                Transform3D local = p >= 0 ? world[p].AffineInverse() * world[b] : world[b];

                int at = b * frames + f;
                positions[at] = local.Origin;
                rotations[at] = local.Basis.GetRotationQuaternion();
                scales[at] = local.Basis.Scale;
            }
        }

        var clip = new Animation
        {
            Length = MathF.Max(seconds, 1.0f / Fps),
            LoopMode = source.Loops ? Animation.LoopModeEnum.Linear : Animation.LoopModeEnum.None,
        };

        for (int b = 0; b < bones; b++)
        {
            string path = skeletonName + ":" + M3SceneBuilder.BoneName(b);
            Transform3D rest = skeleton.GetBoneRest(b);
            int at = b * frames;

            keys += AddPosition(clip, path, positions, at, frames, rest.Origin, seconds);
            keys += AddRotation(clip, path, rotations, at, frames,
                                rest.Basis.GetRotationQuaternion(), seconds);
            keys += AddScale(clip, path, scales, at, frames, rest.Basis.Scale, seconds);
        }

        return clip.GetTrackCount() == 0 ? null : clip;
    }

    private static int AddPosition(Animation clip, string path, Vector3[] values, int at,
                                   int frames, Vector3 rest, float seconds)
    {
        bool constant = IsConstant(values, at, frames, PositionEpsilon);
        if (constant && (values[at] - rest).Length() <= PositionEpsilon)
        {
            return 0;
        }

        int track = clip.AddTrack(Animation.TrackType.Position3D);
        clip.TrackSetPath(track, path);

        if (constant)
        {
            clip.PositionTrackInsertKey(track, 0.0, values[at]);
            return 1;
        }

        for (int f = 0; f < frames; f++)
        {
            clip.PositionTrackInsertKey(track, Time(f, frames, seconds), values[at + f]);
        }

        return frames;
    }

    private static int AddRotation(Animation clip, string path, Quaternion[] values, int at,
                                   int frames, Quaternion rest, float seconds)
    {
        bool constant = IsConstant(values, at, frames, RotationEpsilon);
        if (constant && values[at].AngleTo(rest) <= RotationEpsilon)
        {
            return 0;
        }

        int track = clip.AddTrack(Animation.TrackType.Rotation3D);
        clip.TrackSetPath(track, path);

        if (constant)
        {
            clip.RotationTrackInsertKey(track, 0.0, values[at]);
            return 1;
        }

        for (int f = 0; f < frames; f++)
        {
            clip.RotationTrackInsertKey(track, Time(f, frames, seconds), values[at + f]);
        }

        return frames;
    }

    private static int AddScale(Animation clip, string path, Vector3[] values, int at,
                                int frames, Vector3 rest, float seconds)
    {
        bool constant = IsConstant(values, at, frames, ScaleEpsilon);
        if (constant && (values[at] - rest).Length() <= ScaleEpsilon)
        {
            return 0;
        }

        int track = clip.AddTrack(Animation.TrackType.Scale3D);
        clip.TrackSetPath(track, path);

        if (constant)
        {
            clip.ScaleTrackInsertKey(track, 0.0, values[at]);
            return 1;
        }

        for (int f = 0; f < frames; f++)
        {
            clip.ScaleTrackInsertKey(track, Time(f, frames, seconds), values[at + f]);
        }

        return frames;
    }

    private static double Time(int frame, int frames, float seconds) =>
        (double)frame / (frames - 1) * seconds;

    private static bool IsConstant(Vector3[] values, int at, int frames, float epsilon)
    {
        for (int f = 1; f < frames; f++)
        {
            if ((values[at + f] - values[at]).Length() > epsilon)
            {
                return false;
            }
        }

        return true;
    }

    private static bool IsConstant(Quaternion[] values, int at, int frames, float epsilon)
    {
        for (int f = 1; f < frames; f++)
        {
            if (values[at + f].AngleTo(values[at]) > epsilon)
            {
                return false;
            }
        }

        return true;
    }
}
