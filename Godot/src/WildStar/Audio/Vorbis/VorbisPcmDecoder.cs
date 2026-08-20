using System;
using System.Collections.Generic;
using System.IO;
using NVorbis;

namespace WildStar.Audio.Vorbis;

internal static class VorbisPcmDecoder
{
    private const int BlockSamples = 16384;

    public static short[] Decode(byte[] ogg, out int channels, out int sampleRate)
    {
        using var source = new MemoryStream(ogg, false);
        using var reader = new VorbisReader(source, false);

        channels = reader.Channels;
        sampleRate = reader.SampleRate;

        if (channels <= 0 || sampleRate <= 0)
        {
            throw new InvalidDataException("rebuilt Ogg declares no channels or no sample rate");
        }

        var blocks = new List<short[]>();
        var window = new float[BlockSamples * channels];
        int total = 0;

        while (true)
        {
            int read = reader.ReadSamples(window, 0, window.Length);
            if (read <= 0)
            {
                break;
            }

            var block = new short[read];
            for (int i = 0; i < read; i++)
            {
                block[i] = ToPcm(window[i]);
            }

            blocks.Add(block);
            total += read;
        }

        var samples = new short[total];
        int cursor = 0;
        foreach (short[] block in blocks)
        {
            Array.Copy(block, 0, samples, cursor, block.Length);
            cursor += block.Length;
        }

        return samples;
    }

    private static short ToPcm(float sample)
    {
        int scaled = (int)MathF.Round(sample * 32767f);
        return (short)Math.Clamp(scaled, short.MinValue, short.MaxValue);
    }
}
