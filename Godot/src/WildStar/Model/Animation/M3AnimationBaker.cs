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
    public const string SourceIndexMeta = "m3_source_animation_index";

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

        var library = new AnimationLibrary();
        var used = new HashSet<string>(StringComparer.Ordinal);
        int registered = 0;
        int skipped = 0;

        for (int index = 0; index < model.Animations.Length; index++)
        {
            M3Animation source = model.Animations[index];
            if (source.Duration == 0)
            {
                skipped++;
                continue;
            }

            var clip = new Animation
            {
                Length = MathF.Max(source.Seconds, 0.001f),
                LoopMode = source.Loops
                    ? Animation.LoopModeEnum.Linear
                    : Animation.LoopModeEnum.None,
            };
            clip.SetMeta(SourceIndexMeta, index);

            library.AddAnimation(
                UniqueName(source, index, used, sequenceNames, variations), clip);
            registered++;
        }

        if (registered == 0)
        {
            return new M3BakeResult(null, 0, skipped, 0);
        }

        var player = new AnimationPlayer
        {
            Name = "AnimationPlayer",
            RootNode = new NodePath(".."),
        };
        player.AddAnimationLibrary(string.Empty, library);
        return new M3BakeResult(player, registered, skipped, 0);
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
}
