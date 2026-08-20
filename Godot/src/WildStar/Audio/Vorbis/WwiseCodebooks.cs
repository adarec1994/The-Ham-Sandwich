using System;
using System.Buffers.Binary;
using System.IO;
using System.Reflection;

namespace WildStar.Audio.Vorbis;

internal sealed class WwiseCodebooks
{
    private const string ResourceName = "WildStar.Audio.PackedCodebooks.bin";

    private static readonly Lazy<WwiseCodebooks> Embedded = new(LoadEmbedded);

    private readonly byte[] _data;
    private readonly int[] _offsets;

    private WwiseCodebooks(byte[] data, int[] offsets)
    {
        _data = data;
        _offsets = offsets;
    }

    public static WwiseCodebooks Default => Embedded.Value;

    public int Count => Math.Max(0, _offsets.Length - 1);

    public ReadOnlyMemory<byte> Get(int id)
    {
        if (id < 0 || id >= Count)
        {
            throw new InvalidDataException("codebook " + id + " is outside the packed library");
        }

        int start = _offsets[id];
        int length = _offsets[id + 1] - start;

        if (start < 0 || length <= 0 || start + length > _data.Length)
        {
            throw new InvalidDataException("codebook " + id + " has a corrupt offset");
        }

        return _data.AsMemory(start, length);
    }

    public static WwiseCodebooks Load(byte[] packed)
    {
        if (packed.Length < 8)
        {
            throw new InvalidDataException("packed codebook library is too short");
        }

        int tableOffset = (int)BinaryPrimitives.ReadUInt32LittleEndian(
            packed.AsSpan(packed.Length - 4));

        if (tableOffset < 0 || tableOffset > packed.Length - 4)
        {
            throw new InvalidDataException("packed codebook library has a corrupt offset table");
        }

        int count = (packed.Length - tableOffset) / 4;
        var offsets = new int[count];
        for (int i = 0; i < count; i++)
        {
            offsets[i] = (int)BinaryPrimitives.ReadUInt32LittleEndian(
                packed.AsSpan(tableOffset + i * 4));
        }

        var data = new byte[tableOffset];
        Array.Copy(packed, data, tableOffset);

        return new WwiseCodebooks(data, offsets);
    }

    private static WwiseCodebooks LoadEmbedded()
    {
        using Stream? stream = typeof(WwiseCodebooks).GetTypeInfo().Assembly
            .GetManifestResourceStream(ResourceName);

        if (stream is null)
        {
            throw new InvalidDataException("embedded resource " + ResourceName + " is missing");
        }

        using var buffer = new MemoryStream();
        stream.CopyTo(buffer);
        return Load(buffer.ToArray());
    }
}
