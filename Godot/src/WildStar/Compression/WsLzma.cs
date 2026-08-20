using System;
using System.IO;

namespace WildStar.Compression;

internal static class WsLzma
{
    private const int PropertySize = 5;

    public static byte[] Decode(byte[] raw, int expected, string what)
    {
        if (raw.Length <= PropertySize)
        {
            throw new InvalidDataException(what + ": LZMA block is only " + raw.Length + " bytes");
        }

        var properties = new byte[PropertySize];
        Array.Copy(raw, properties, PropertySize);

        var decoder = new SevenZip.Compression.LZMA.Decoder();
        decoder.SetDecoderProperties(properties);

        using var source = new MemoryStream(raw, PropertySize, raw.Length - PropertySize, false);
        using var destination = new MemoryStream(expected);

        try
        {
            decoder.Code(source, destination, raw.Length - PropertySize, expected, null!);
        }
        catch (Exception) when (destination.Length > 0)
        {
        }

        return destination.ToArray();
    }
}
