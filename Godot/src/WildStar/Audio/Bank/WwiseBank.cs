using System;
using System.Buffers.Binary;
using System.Collections.Generic;
using System.Text;

namespace WildStar.Audio.Bank;

public sealed class WwiseBankSound
{
    internal WwiseBankSound(WwiseBank bank, uint id, int offset, int length, bool prefetch)
    {
        Bank = bank;
        Id = id;
        Offset = offset;
        Length = length;
        IsPrefetch = prefetch;
    }

    public WwiseBank Bank { get; }

    public uint Id { get; }

    public int Offset { get; }

    public int Length { get; }

    public bool IsPrefetch { get; }

    public string Name => Id + ".wem";

    public byte[] ReadAllBytes() => Bank.Read(this);
}

public sealed class WwiseBank
{
    private readonly byte[] _bytes;
    private readonly int _dataOffset;

    private WwiseBank(byte[] bytes, uint version, int dataOffset, List<WwiseBankSound> sounds)
    {
        _bytes = bytes;
        _dataOffset = dataOffset;
        Version = version;
        Sounds = sounds;
    }

    public uint Version { get; }

    public IReadOnlyList<WwiseBankSound> Sounds { get; }

    public static bool IsBank(ReadOnlySpan<byte> bytes) =>
        bytes.Length >= 8 && Encoding.ASCII.GetString(bytes[..4]) == "BKHD";

    public static bool TryParse(byte[] bytes, out WwiseBank bank, out string error)
    {
        bank = null!;

        if (!IsBank(bytes))
        {
            error = "no BKHD header";
            return false;
        }

        uint version = 0;
        int dataOffset = -1;
        int dataSize = 0;
        int didxOffset = -1;
        int didxSize = 0;

        int offset = 0;
        while (offset + 8 <= bytes.Length)
        {
            string id = Encoding.ASCII.GetString(bytes, offset, 4);
            uint size = BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(offset + 4));
            int body = offset + 8;

            if (size > (uint)(bytes.Length - body))
            {
                break;
            }

            switch (id)
            {
                case "BKHD":
                    if (size >= 4)
                    {
                        version = BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(body));
                    }

                    break;
                case "DIDX":
                    didxOffset = body;
                    didxSize = (int)size;
                    break;
                case "DATA":
                    dataOffset = body;
                    dataSize = (int)size;
                    break;
            }

            offset = body + (int)size;
        }

        var sounds = new List<WwiseBankSound>();

        if (didxOffset < 0 || dataOffset < 0)
        {
            bank = new WwiseBank(bytes, version, Math.Max(dataOffset, 0), sounds);
            error = string.Empty;
            return true;
        }

        var parsed = new WwiseBank(bytes, version, dataOffset, sounds);

        for (int i = 0; i + 12 <= didxSize; i += 12)
        {
            int entry = didxOffset + i;
            uint soundId = BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(entry));
            int soundOffset = (int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(entry + 4));
            int soundLength = (int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(entry + 8));

            if (soundOffset < 0 || soundLength <= 0 || soundOffset + soundLength > dataSize)
            {
                continue;
            }

            sounds.Add(new WwiseBankSound(parsed, soundId, soundOffset, soundLength,
                IsPrefetch(bytes, dataOffset + soundOffset, soundLength)));
        }

        sounds.Sort(static (a, b) => a.Id.CompareTo(b.Id));

        bank = parsed;
        error = string.Empty;
        return true;
    }

    public bool TryGetSound(uint id, out WwiseBankSound sound)
    {
        foreach (WwiseBankSound candidate in Sounds)
        {
            if (candidate.Id == id)
            {
                sound = candidate;
                return true;
            }
        }

        sound = null!;
        return false;
    }

    private static bool IsPrefetch(byte[] bytes, int offset, int length)
    {
        if (length < 12 || Encoding.ASCII.GetString(bytes, offset, 4) != "RIFF")
        {
            return false;
        }

        long declared = (long)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(offset + 4)) + 8;
        return declared > length;
    }

    internal byte[] Read(WwiseBankSound sound)
    {
        var bytes = new byte[sound.Length];
        Array.Copy(_bytes, _dataOffset + sound.Offset, bytes, 0, sound.Length);
        return bytes;
    }
}
