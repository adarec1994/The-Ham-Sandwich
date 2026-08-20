using System;

namespace WildStar.Audio.Vorbis;

internal sealed class VorbisBitReader
{
    private readonly ReadOnlyMemory<byte> _data;
    private long _bit;

    public VorbisBitReader(ReadOnlyMemory<byte> data) => _data = data;

    public long BitsRead => _bit;

    public uint Read(int bits)
    {
        uint result = 0;
        ReadOnlySpan<byte> span = _data.Span;

        for (int i = 0; i < bits; i++)
        {
            long index = _bit >> 3;
            if (index < span.Length && (span[(int)index] & (1 << (int)(_bit & 7))) != 0)
            {
                result |= 1u << i;
            }

            _bit++;
        }

        return result;
    }
}
