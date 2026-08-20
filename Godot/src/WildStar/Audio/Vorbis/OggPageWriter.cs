using System;
using System.Collections.Generic;
using System.IO;

namespace WildStar.Audio.Vorbis;

internal sealed class OggPageWriter
{
    private const int MaxSegments = 255;
    private const int SegmentSize = 255;

    private static readonly uint[] CrcTable = BuildCrcTable();

    private readonly List<byte> _output = new();
    private readonly List<byte> _payload = new();
    private ulong _granule;
    private uint _sequence;
    private byte _partial;
    private int _partialBits;

    public byte[] ToArray() => _output.ToArray();

    public void SetGranule(ulong granule) => _granule = granule;

    public void WriteBit(bool set)
    {
        if (set)
        {
            _partial |= (byte)(1 << _partialBits);
        }

        if (++_partialBits == 8)
        {
            FlushBits();
        }
    }

    public void WriteBits(uint value, int bits)
    {
        for (int i = 0; i < bits; i++)
        {
            WriteBit((value & (1u << i)) != 0);
        }
    }

    public void WriteBytes(ReadOnlySpan<byte> bytes)
    {
        foreach (byte b in bytes)
        {
            WriteBits(b, 8);
        }
    }

    public void FlushBits()
    {
        if (_partialBits == 0)
        {
            return;
        }

        _payload.Add(_partial);
        _partial = 0;
        _partialBits = 0;
    }

    public void FlushPage(bool continued = false, bool first = false, bool last = false)
    {
        FlushBits();

        if (_payload.Count == 0)
        {
            return;
        }

        int payloadSize = _payload.Count;
        int segments = payloadSize / SegmentSize + 1;

        if (segments > MaxSegments)
        {
            throw new InvalidDataException(
                "Ogg packet of " + payloadSize + " bytes needs more than one page");
        }

        int header = _output.Count;

        _output.Add((byte)'O');
        _output.Add((byte)'g');
        _output.Add((byte)'g');
        _output.Add((byte)'S');
        _output.Add(0);

        byte flags = 0;
        if (continued)
        {
            flags |= 0x01;
        }

        if (first)
        {
            flags |= 0x02;
        }

        if (last)
        {
            flags |= 0x04;
        }

        _output.Add(flags);

        for (int i = 0; i < 8; i++)
        {
            _output.Add((byte)(_granule >> (i * 8)));
        }

        _output.Add(1);
        _output.Add(0);
        _output.Add(0);
        _output.Add(0);

        for (int i = 0; i < 4; i++)
        {
            _output.Add((byte)(_sequence >> (i * 8)));
        }

        _sequence++;

        int crcOffset = _output.Count;
        _output.Add(0);
        _output.Add(0);
        _output.Add(0);
        _output.Add(0);

        _output.Add((byte)segments);

        int remaining = payloadSize;
        for (int i = 0; i < segments; i++)
        {
            byte lace = (byte)Math.Min(remaining, SegmentSize);
            _output.Add(lace);
            remaining -= lace;
        }

        _output.AddRange(_payload);
        _payload.Clear();

        uint crc = 0;
        for (int i = header; i < _output.Count; i++)
        {
            crc = (crc << 8) ^ CrcTable[((crc >> 24) & 0xFF) ^ _output[i]];
        }

        _output[crcOffset + 0] = (byte)crc;
        _output[crcOffset + 1] = (byte)(crc >> 8);
        _output[crcOffset + 2] = (byte)(crc >> 16);
        _output[crcOffset + 3] = (byte)(crc >> 24);
    }

    private static uint[] BuildCrcTable()
    {
        var table = new uint[256];

        for (uint i = 0; i < 256; i++)
        {
            uint crc = i << 24;
            for (int bit = 0; bit < 8; bit++)
            {
                crc = (crc & 0x80000000) != 0 ? (crc << 1) ^ 0x04C11DB7 : crc << 1;
            }

            table[i] = crc;
        }

        return table;
    }
}
