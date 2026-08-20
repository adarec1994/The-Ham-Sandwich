using System;

namespace WildStar.Model;

public sealed class M3RawTrack
{
    public static readonly M3RawTrack Empty = new(Array.Empty<uint>(), Array.Empty<byte>(), 0);

    public M3RawTrack(uint[] keys, byte[] values, int valueSize)
    {
        Keys = keys;
        Values = values;
        ValueSize = valueSize;
    }

    public uint[] Keys { get; }

    public byte[] Values { get; }

    public int ValueSize { get; }

    public int Count => Keys.Length;

    public bool HasKeys => Keys.Length != 0;
}
