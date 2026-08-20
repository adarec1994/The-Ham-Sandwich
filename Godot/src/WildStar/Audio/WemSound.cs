namespace WildStar.Audio;

public sealed class WemSound
{
    internal WemSound(short[] samples, int channels, int sampleRate, WemCodec codec,
        int sourceChannels)
    {
        Samples = samples;
        Channels = channels;
        SampleRate = sampleRate;
        SourceCodec = codec;
        SourceChannels = sourceChannels;
    }

    public short[] Samples { get; }

    public int Channels { get; }

    public int SampleRate { get; }

    public WemCodec SourceCodec { get; }

    public int SourceChannels { get; }

    public bool WasFoldedToStereo => SourceChannels != Channels;

    public int Frames => Channels == 0 ? 0 : Samples.Length / Channels;

    public double Seconds => SampleRate == 0 ? 0 : (double)Frames / SampleRate;
}
