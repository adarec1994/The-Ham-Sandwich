using System;
using System.IO;
using WildStar.Audio.Adpcm;
using WildStar.Audio.Vorbis;

namespace WildStar.Audio;

public static class WemDecoder
{
    public static WemSound Decode(byte[] bytes)
    {
        if (!WemFile.TryParse(bytes, out WemFile wem, out string error))
        {
            throw new InvalidDataException("not a usable WEM: " + error);
        }

        return Decode(wem);
    }

    public static WemSound Decode(WemFile wem)
    {
        short[] samples;
        int channels = wem.Channels;
        int sampleRate = wem.SampleRate;

        switch (wem.Codec)
        {
            case WemCodec.WwiseVorbis:
            case WemCodec.WwiseVorbisAlt:
                samples = VorbisPcmDecoder.Decode(
                    WwiseVorbisRebuilder.ToOgg(wem), out channels, out sampleRate);
                break;

            case WemCodec.WwiseAdpcm:
                samples = WwiseAdpcmDecoder.Decode(wem);
                break;

            case WemCodec.Pcm when wem.BitsPerSample == 16:
                samples = ReadPcm16(wem);
                break;

            default:
                throw new InvalidDataException(
                    "unsupported WEM codec 0x" + ((ushort)wem.Codec).ToString("X4") +
                    " at " + wem.BitsPerSample + " bits");
        }

        int sourceChannels = channels;

        if (channels > 2)
        {
            samples = FoldToStereo(samples, channels);
            channels = 2;
        }

        return new WemSound(samples, channels, sampleRate, wem.Codec, sourceChannels);
    }

    public static bool TryDecode(byte[] bytes, out WemSound sound, out string error)
    {
        try
        {
            sound = Decode(bytes);
            error = string.Empty;
            return true;
        }
        catch (Exception exception)
        {
            sound = null!;
            error = exception.Message;
            return false;
        }
    }

    private static short[] FoldToStereo(short[] samples, int channels)
    {
        int frames = samples.Length / channels;
        var folded = new short[frames * 2];

        int lefts = (channels + 1) / 2;
        int rights = channels / 2;

        for (int frame = 0; frame < frames; frame++)
        {
            int source = frame * channels;
            int left = 0;
            int right = 0;

            for (int channel = 0; channel < channels; channel++)
            {
                if ((channel & 1) == 0)
                {
                    left += samples[source + channel];
                }
                else
                {
                    right += samples[source + channel];
                }
            }

            folded[frame * 2] = (short)Math.Clamp(left / lefts, short.MinValue, short.MaxValue);
            folded[frame * 2 + 1] = rights == 0
                ? folded[frame * 2]
                : (short)Math.Clamp(right / rights, short.MinValue, short.MaxValue);
        }

        return folded;
    }

    private static short[] ReadPcm16(WemFile wem)
    {
        int end = Math.Min(wem.DataOffset + wem.DataSize, wem.Bytes.Length);
        var samples = new short[(end - wem.DataOffset) / 2];
        Buffer.BlockCopy(wem.Bytes, wem.DataOffset, samples, 0, samples.Length * 2);
        return samples;
    }
}
