using System;
using System.IO;

namespace WildStar.Audio.Adpcm;

internal static class WwiseAdpcmDecoder
{
    private static readonly int[] IndexTable =
        { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };

    private static readonly int[] StepTable =
    {
        7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
        50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
        337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552,
        1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
        7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385,
        24623, 27086, 29794, 32767,
    };

    public static short[] Decode(WemFile wem)
    {
        int channels = wem.Channels;
        int blockAlign = wem.BlockAlign;

        if (channels <= 0 || blockAlign <= 0 || blockAlign % channels != 0)
        {
            throw new InvalidDataException(
                "Wwise ADPCM needs a block size divisible by the channel count");
        }

        int bytesPerChannel = blockAlign / channels;
        if (bytesPerChannel <= 4)
        {
            throw new InvalidDataException("Wwise ADPCM block holds no sample data");
        }

        int dataEnd = Math.Min(wem.DataOffset + wem.DataSize, wem.Bytes.Length);
        int blocks = (dataEnd - wem.DataOffset) / blockAlign;
        int samplesPerBlock = 1 + (bytesPerChannel - 4) * 2;

        var output = new short[blocks * samplesPerBlock * channels];
        var channelSamples = new short[channels][];
        for (int i = 0; i < channels; i++)
        {
            channelSamples[i] = new short[samplesPerBlock];
        }

        ReadOnlySpan<byte> bytes = wem.Bytes;
        int written = 0;

        for (int block = 0; block < blocks; block++)
        {
            int blockStart = wem.DataOffset + block * blockAlign;

            for (int channel = 0; channel < channels; channel++)
            {
                ReadOnlySpan<byte> source = bytes.Slice(blockStart + channel * bytesPerChannel,
                    bytesPerChannel);

                int predictor = (short)(source[0] | (source[1] << 8));
                int stepIndex = Math.Min((int)source[2], 88);

                short[] target = channelSamples[channel];
                target[0] = (short)predictor;

                int index = 1;
                for (int i = 4; i < bytesPerChannel; i++)
                {
                    byte packed = source[i];
                    target[index++] = Step(packed & 0x0F, ref predictor, ref stepIndex);
                    target[index++] = Step((packed >> 4) & 0x0F, ref predictor, ref stepIndex);
                }
            }

            for (int sample = 0; sample < samplesPerBlock; sample++)
            {
                for (int channel = 0; channel < channels; channel++)
                {
                    output[written++] = channelSamples[channel][sample];
                }
            }
        }

        return output;
    }

    private static short Step(int nibble, ref int predictor, ref int stepIndex)
    {
        int step = StepTable[stepIndex];
        int difference = step >> 3;

        if ((nibble & 1) != 0)
        {
            difference += step >> 2;
        }

        if ((nibble & 2) != 0)
        {
            difference += step >> 1;
        }

        if ((nibble & 4) != 0)
        {
            difference += step;
        }

        if ((nibble & 8) != 0)
        {
            difference = -difference;
        }

        predictor = Math.Clamp(predictor + difference, short.MinValue, short.MaxValue);
        stepIndex = Math.Clamp(stepIndex + IndexTable[nibble], 0, 88);

        return (short)predictor;
    }
}
