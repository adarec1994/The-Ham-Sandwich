namespace WildStar.Audio;

public enum WemCodec : ushort
{
    Pcm = 0x0001,
    WwiseAdpcm = 0x0002,
    ImaAdpcm = 0x0011,
    WwiseVorbis = 0xFFFF,
    WwiseVorbisAlt = 0xFFFE,
}
