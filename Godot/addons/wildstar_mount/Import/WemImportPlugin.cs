#if TOOLS
using System;
using Godot;
using Godot.Collections;
using WildStar.Audio;
using WildStar.Playback;

namespace WildStar.Editor;

[Tool]
public partial class WemImportPlugin : EditorImportPlugin
{
    private const string OptionForceMono = "force/mono";
    private const string OptionNormalize = "edit/normalize";
    private const string OptionLoopMode = "edit/loop_mode";
    private const string OptionLoopBegin = "edit/loop_begin";
    private const string OptionLoopEnd = "edit/loop_end";

    private const string LoopModes =
        "Disabled,Forward,Ping-Pong,Backward";

    public override string _GetImporterName() => "wildstar.wem";

    public override string _GetVisibleName() => "WildStar Wwise Audio";

    public override string[] _GetRecognizedExtensions() => new[] { "wem" };

    public override string _GetSaveExtension() => "sample";

    public override string _GetResourceType() => "AudioStreamWAV";

    public override float _GetPriority() => 1.0f;

    public override int _GetImportOrder() => 0;

    public override int _GetPresetCount() => 1;

    public override string _GetPresetName(int presetIndex) => "Default";

    public override Array<Dictionary> _GetImportOptions(string path, int presetIndex) => new()
    {
        Option(OptionForceMono, false),
        Option(OptionNormalize, false),
        Option(OptionLoopMode, 0, PropertyHint.Enum, LoopModes),
        Option(OptionLoopBegin, 0),
        Option(OptionLoopEnd, -1),
    };

    public override bool _GetOptionVisibility(string path, StringName optionName,
        Dictionary options) => true;

    public override Error _Import(string sourceFile, string savePath, Dictionary options,
        Array<string> platformVariants, Array<string> genFiles)
    {
        byte[] bytes = FileAccess.GetFileAsBytes(sourceFile);
        if (bytes.Length == 0)
        {
            GD.PushError("[wildstar_mount] " + sourceFile + ": " +
                FileAccess.GetOpenError() + " while reading");
            return Error.CantOpen;
        }

        if (!WemDecoder.TryDecode(bytes, out WemSound sound, out string error))
        {
            GD.PushError("[wildstar_mount] " + sourceFile + ": " + error);
            return Error.FileUnrecognized;
        }

        if (Get(options, OptionForceMono, false).AsBool() && sound.Channels == 2)
        {
            sound = ToMono(sound);
        }

        if (Get(options, OptionNormalize, false).AsBool())
        {
            Normalize(sound.Samples);
        }

        AudioStreamWav stream = WemStreamFactory.Create(sound);
        ApplyLoop(stream, options, sound.Frames);

        return ResourceSaver.Save(stream, savePath + "." + _GetSaveExtension());
    }

    private static void ApplyLoop(AudioStreamWav stream, Dictionary options, int frames)
    {
        var mode = (AudioStreamWav.LoopModeEnum)Get(options, OptionLoopMode, 0).AsInt32();
        stream.LoopMode = mode;

        if (mode == AudioStreamWav.LoopModeEnum.Disabled)
        {
            return;
        }

        int begin = Math.Clamp(Get(options, OptionLoopBegin, 0).AsInt32(), 0, frames);
        int end = Get(options, OptionLoopEnd, -1).AsInt32();

        stream.LoopBegin = begin;
        stream.LoopEnd = end < 0 ? frames : Math.Clamp(end, begin, frames);
    }

    private static WemSound ToMono(WemSound sound)
    {
        int frames = sound.Frames;
        var mono = new short[frames];

        for (int frame = 0; frame < frames; frame++)
        {
            mono[frame] = (short)((sound.Samples[frame * 2] + sound.Samples[frame * 2 + 1]) / 2);
        }

        return new WemSound(mono, 1, sound.SampleRate, sound.SourceCodec, sound.SourceChannels);
    }

    private static void Normalize(short[] samples)
    {
        int peak = 0;
        foreach (short sample in samples)
        {
            peak = Math.Max(peak, Math.Abs((int)sample));
        }

        if (peak == 0 || peak >= short.MaxValue)
        {
            return;
        }

        double gain = (double)short.MaxValue / peak;
        for (int i = 0; i < samples.Length; i++)
        {
            samples[i] = (short)Math.Clamp(
                (int)Math.Round(samples[i] * gain), short.MinValue, short.MaxValue);
        }
    }

    private static Variant Get(Dictionary options, string name, Variant fallback) =>
        options.TryGetValue(name, out Variant value) ? value : fallback;

    private static Dictionary Option(string name, Variant defaultValue,
        PropertyHint hint = PropertyHint.None, string hintString = "") => new()
    {
        { "name", name },
        { "default_value", defaultValue },
        { "property_hint", (int)hint },
        { "hint_string", hintString },
    };
}
#endif
