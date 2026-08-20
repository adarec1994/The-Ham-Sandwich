using System;
using Godot;
using WildStar.Audio;

namespace WildStar.Playback;

public static class WemStreamFactory
{
    public static bool TryCreate(byte[] wemBytes, out AudioStreamWav stream, out string error)
    {
        if (!WemDecoder.TryDecode(wemBytes, out WemSound sound, out error))
        {
            stream = null!;
            return false;
        }

        return TryCreate(sound, out stream, out error);
    }

    public static bool TryCreate(WemSound sound, out AudioStreamWav stream, out string error)
    {
        stream = null!;

        if (sound.Channels is not (1 or 2))
        {
            error = "AudioStreamWav holds mono or stereo only, not " + sound.Channels + " channels";
            return false;
        }

        stream = Create(sound);
        error = string.Empty;
        return true;
    }

    public static AudioStreamWav Create(WemSound sound)
    {
        var bytes = new byte[sound.Samples.Length * 2];
        Buffer.BlockCopy(sound.Samples, 0, bytes, 0, bytes.Length);

        return new AudioStreamWav
        {
            Data = bytes,
            Format = AudioStreamWav.FormatEnum.Format16Bits,
            MixRate = sound.SampleRate,
            Stereo = sound.Channels == 2,
        };
    }

    public static bool IsWem(string path) =>
        path.EndsWith(".wem", StringComparison.OrdinalIgnoreCase);
}
