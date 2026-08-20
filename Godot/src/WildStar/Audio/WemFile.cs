using System;
using System.Buffers.Binary;
using System.Text;

namespace WildStar.Audio;

public sealed class WemFile
{
    private WemFile(byte[] bytes, bool littleEndian)
    {
        Bytes = bytes;
        LittleEndian = littleEndian;
    }

    public byte[] Bytes { get; }

    public bool LittleEndian { get; }

    public WemCodec Codec { get; private set; }

    public int Channels { get; private set; }

    public int SampleRate { get; private set; }

    public int AverageBytesPerSecond { get; private set; }

    public int BlockAlign { get; private set; }

    public int BitsPerSample { get; private set; }

    public int FormatOffset { get; private set; } = -1;

    public int FormatSize { get; private set; }

    public int VorbOffset { get; private set; } = -1;

    public int VorbSize { get; private set; }

    public int DataOffset { get; private set; } = -1;

    public int DataSize { get; private set; }

    public static bool IsWem(ReadOnlySpan<byte> bytes) =>
        bytes.Length >= 12 &&
        (Tag(bytes) == "RIFF" || Tag(bytes) == "RIFX") &&
        Tag(bytes[8..]) == "WAVE";

    public static bool TryParse(byte[] bytes, out WemFile file, out string error)
    {
        file = null!;

        if (bytes.Length < 12)
        {
            error = "shorter than a RIFF header";
            return false;
        }

        string riff = Tag(bytes);
        if (riff != "RIFF" && riff != "RIFX")
        {
            error = "not RIFF or RIFX";
            return false;
        }

        if (Tag(bytes.AsSpan(8)) != "WAVE")
        {
            error = "not a WAVE container";
            return false;
        }

        var parsed = new WemFile(bytes, riff == "RIFF");

        long declared = (long)parsed.ReadUInt32(4) + 8;
        int end = (int)Math.Min(declared, bytes.Length);

        int offset = 12;
        while (offset + 8 <= end)
        {
            string id = Tag(bytes.AsSpan(offset));
            uint size = parsed.ReadUInt32(offset + 4);
            int body = offset + 8;

            if (size > (uint)(end - body))
            {
                break;
            }

            switch (id)
            {
                case "fmt ":
                    parsed.FormatOffset = body;
                    parsed.FormatSize = (int)size;
                    break;
                case "vorb":
                    parsed.VorbOffset = body;
                    parsed.VorbSize = (int)size;
                    break;
                case "data":
                    parsed.DataOffset = body;
                    parsed.DataSize = (int)size;
                    break;
            }

            offset = body + (int)size;
        }

        if (parsed.FormatOffset < 0 || parsed.FormatSize < 16)
        {
            error = "no fmt chunk";
            return false;
        }

        if (parsed.DataOffset < 0)
        {
            error = "no data chunk";
            return false;
        }

        parsed.Codec = (WemCodec)parsed.ReadUInt16(parsed.FormatOffset);
        parsed.Channels = parsed.ReadUInt16(parsed.FormatOffset + 2);
        parsed.SampleRate = (int)parsed.ReadUInt32(parsed.FormatOffset + 4);
        parsed.AverageBytesPerSecond = (int)parsed.ReadUInt32(parsed.FormatOffset + 8);
        parsed.BlockAlign = parsed.ReadUInt16(parsed.FormatOffset + 12);
        parsed.BitsPerSample = parsed.ReadUInt16(parsed.FormatOffset + 14);

        if (parsed.VorbOffset < 0 && parsed.FormatSize == 0x42)
        {
            parsed.VorbOffset = parsed.FormatOffset + 0x18;
            parsed.VorbSize = 0x2A;
        }

        if (parsed.Channels == 0 || parsed.SampleRate == 0)
        {
            error = "fmt chunk declares no channels or no sample rate";
            return false;
        }

        file = parsed;
        error = string.Empty;
        return true;
    }

    public ushort ReadUInt16(int offset) => LittleEndian
        ? BinaryPrimitives.ReadUInt16LittleEndian(Bytes.AsSpan(offset))
        : BinaryPrimitives.ReadUInt16BigEndian(Bytes.AsSpan(offset));

    public uint ReadUInt32(int offset) => LittleEndian
        ? BinaryPrimitives.ReadUInt32LittleEndian(Bytes.AsSpan(offset))
        : BinaryPrimitives.ReadUInt32BigEndian(Bytes.AsSpan(offset));

    private static string Tag(ReadOnlySpan<byte> bytes) => Encoding.ASCII.GetString(bytes[..4]);
}
