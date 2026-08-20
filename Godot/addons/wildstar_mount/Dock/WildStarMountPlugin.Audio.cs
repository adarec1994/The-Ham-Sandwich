#if TOOLS
using System;
using Godot;
using WildStar.Archive;
using WildStar.Audio;
using WildStar.Playback;

namespace WildStar.Editor;

public partial class WildStarMountPlugin
{
    private AudioStreamPlayer? _preview;
    private AudioStream? _selected;
    private string _selectedLabel = string.Empty;

    private static bool IsAudio(string name) =>
        name.EndsWith(".wem", StringComparison.OrdinalIgnoreCase);

    private void OnSelected(TreeItem item)
    {
        string meta = item.GetMetadata(0).AsString();

        if (meta.StartsWith(BankSoundPrefix, StringComparison.Ordinal))
        {
            string label = meta[BankSoundPrefix.Length..];
            if (TryGetBankSoundBytes(label, out byte[] audio))
            {
                Inspect(label, audio);
            }

            return;
        }

        if (!TryGetFileMeta(meta, out WsFile file) || !IsAudio(file.Name))
        {
            return;
        }

        try
        {
            Inspect(file.QualifiedPath, file.ReadAllBytes());
        }
        catch (Exception exception)
        {
            GD.PushError("[wildstar_mount] " + file.QualifiedPath + ": " + exception.Message);
        }
    }

    private void Inspect(string label, byte[] bytes)
    {
        AudioStream? stream = Resolve(label, bytes);
        if (stream is null)
        {
            return;
        }

        EditorInterface.Singleton.EditResource(stream);
    }

    private void PlayPreview(WsFile file)
    {
        try
        {
            PlayPreview(file.QualifiedPath, file.ReadAllBytes());
        }
        catch (Exception exception)
        {
            GD.PushError("[wildstar_mount] " + file.QualifiedPath + ": " + exception.Message);
        }
    }

    private void PlayPreview(string label, byte[] bytes)
    {
        AudioStream? stream = Resolve(label, bytes);
        if (stream is null)
        {
            return;
        }

        AudioStreamPlayer player = EnsurePreview();
        player.Stream = stream;
        player.Play();

        GD.Print("[wildstar_mount] playing " + label + " — " +
            stream.GetLength().ToString("0.00") + "s");
    }

    private AudioStream? Resolve(string label, byte[] bytes)
    {
        if (_selected is not null && GodotObject.IsInstanceValid(_selected) && _selectedLabel == label)
        {
            return _selected;
        }

        if (!WemDecoder.TryDecode(bytes, out WemSound sound, out string decodeError))
        {
            GD.PushError("[wildstar_mount] " + label + ": " + decodeError);
            return null;
        }

        if (!WemStreamFactory.TryCreate(sound, out AudioStreamWav stream, out string streamError))
        {
            GD.PushError("[wildstar_mount] " + label + ": " + streamError);
            return null;
        }

        stream.ResourceName = label[(label.LastIndexOf('/') + 1)..] + "  ·  " +
            sound.SourceCodec + ", " + sound.Channels + "ch, " + sound.SampleRate + " Hz" +
            (sound.WasFoldedToStereo ? "  (folded from " + sound.SourceChannels + "ch)" : "");

        _selected = stream;
        _selectedLabel = label;
        return stream;
    }

    private void StopPreview()
    {
        if (_preview is not null && GodotObject.IsInstanceValid(_preview))
        {
            _preview.Stop();
        }
    }

    private AudioStreamPlayer EnsurePreview()
    {
        if (_preview is not null && GodotObject.IsInstanceValid(_preview))
        {
            return _preview;
        }

        _preview = new AudioStreamPlayer { Bus = "Master" };
        EditorInterface.Singleton.GetBaseControl().AddChild(_preview);
        return _preview;
    }
}
#endif
