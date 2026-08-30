using System;
using System.Buffers.Binary;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Text;
using WildStar.Compression;

namespace WildStar.Archive;

public sealed class WsArchive : IDisposable
{
    private const uint MagicKcap = 0x4B434150;
    private const uint MagicAidx = 0x58444941;
    private const uint MagicAarc = 0x43524141;

    private const int HeaderSize = 548;
    private const int BlockTableOffsetField = 528;
    private const int BlockCountField = 536;

    private readonly byte[] _index;
    private readonly WsBlock[] _indexBlocks;
    private readonly WsBlock[] _dataBlocks;
    private readonly Dictionary<Sha1Key, uint> _aarc;
    private readonly FileStream _data;
    private readonly object _dataLock = new();
    private bool _disposed;

    private WsArchive(string name, string indexPath, string dataPath, byte[] index,
        WsBlock[] indexBlocks, WsBlock[] dataBlocks, Dictionary<Sha1Key, uint> aarc,
        FileStream data, uint rootBlock)
    {
        Name = name;
        IndexPath = indexPath;
        DataPath = dataPath;
        _index = index;
        _indexBlocks = indexBlocks;
        _dataBlocks = dataBlocks;
        _aarc = aarc;
        _data = data;
        Root = new WsDirectory(this, null, string.Empty, rootBlock);
    }

    public string Name { get; }

    public string IndexPath { get; }

    public string DataPath { get; }

    public WsDirectory Root { get; }

    public int BlockCount => _aarc.Count;

    public static WsArchive Open(string indexPath)
    {
        string dataPath = System.IO.Path.ChangeExtension(indexPath, ".archive");
        if (!File.Exists(dataPath))
        {
            throw new FileNotFoundException(
                System.IO.Path.GetFileName(indexPath) + " has no sibling .archive", dataPath);
        }

        byte[] index = File.ReadAllBytes(indexPath);
        WsBlock[] indexBlocks = ReadBlockTable(index, indexPath);
        uint rootBlock = FindRootBlock(index, indexBlocks, indexPath);

        var data = new FileStream(dataPath, FileMode.Open, FileAccess.Read,
            FileShare.ReadWrite | FileShare.Delete, 1 << 16, FileOptions.RandomAccess);

        try
        {
            WsBlock[] dataBlocks = ReadBlockTable(data, dataPath);
            Dictionary<Sha1Key, uint> aarc = ReadAarcTable(data, dataBlocks, dataPath);

            return new WsArchive(
                System.IO.Path.GetFileNameWithoutExtension(indexPath),
                indexPath, dataPath, index, indexBlocks, dataBlocks, aarc, data, rootBlock);
        }
        catch
        {
            data.Dispose();
            throw;
        }
    }

    public bool TryGetFile(string path, out WsFile file)
    {
        file = null!;
        string[] parts = SplitPath(path);
        if (parts.Length == 0)
        {
            return false;
        }

        WsDirectory directory = Root;
        for (int i = 0; i < parts.Length - 1; i++)
        {
            if (!directory.TryGetDirectory(parts[i], out directory))
            {
                return false;
            }
        }

        return directory.TryGetFile(parts[^1], out file);
    }

    public bool TryGetDirectory(string path, out WsDirectory directory)
    {
        directory = Root;
        foreach (string part in SplitPath(path))
        {
            if (!directory.TryGetDirectory(part, out directory))
            {
                directory = null!;
                return false;
            }
        }

        return true;
    }

    public byte[] ReadFile(WsFile file) =>
        Decompress(ReadRawBlock(file), file.Compression, file.UncompressedSize, file.QualifiedPath);

    public byte[] ReadFilePrefix(WsFile file, int maxBytes)
    {
        if (maxBytes <= 0)
        {
            return Array.Empty<byte>();
        }

        ulong want = Math.Min((ulong)maxBytes, file.UncompressedSize);
        byte[] prefix = Decompress(ReadRawBlock(file), file.Compression, want, file.QualifiedPath);
        return prefix.Length > (int)want ? prefix[..(int)want] : prefix;
    }

    public byte[] ReadRawBlock(WsFile file)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);

        if (!_aarc.TryGetValue(new Sha1Key(file.Hash), out uint blockIndex))
        {
            throw new InvalidDataException(
                file.QualifiedPath + ": no AARC entry for " + Convert.ToHexString(file.Hash));
        }

        if (blockIndex >= _dataBlocks.Length)
        {
            throw new InvalidDataException(
                file.QualifiedPath + ": AARC points at block " + blockIndex +
                " of " + _dataBlocks.Length);
        }

        WsBlock block = _dataBlocks[blockIndex];
        var raw = new byte[Sized(block.Size, file.QualifiedPath)];

        lock (_dataLock)
        {
            _data.Seek((long)block.Offset, SeekOrigin.Begin);
            _data.ReadExactly(raw, 0, raw.Length);
        }

        return raw;
    }

    public void Dispose()
    {
        if (_disposed)
        {
            return;
        }

        _disposed = true;
        _data.Dispose();
    }

    internal void ParseDirectoryBlock(WsDirectory parent, uint blockIndex,
        List<WsDirectory> directories, List<WsFile> files)
    {
        if (blockIndex >= _indexBlocks.Length)
        {
            return;
        }

        WsBlock block = _indexBlocks[blockIndex];
        if (block.Size < 8 || block.Offset + block.Size > (ulong)_index.Length)
        {
            return;
        }

        var span = new ReadOnlySpan<byte>(_index, (int)block.Offset, (int)block.Size);
        uint directoryCount = BinaryPrimitives.ReadUInt32LittleEndian(span);
        uint fileCount = BinaryPrimitives.ReadUInt32LittleEndian(span[4..]);

        long recordBytes = (long)directoryCount * 8 + (long)fileCount * 56;
        if (8 + recordBytes > span.Length)
        {
            return;
        }

        ReadOnlySpan<byte> names = span[(int)(8 + recordBytes)..];

        int cursor = 8;
        for (uint i = 0; i < directoryCount; i++, cursor += 8)
        {
            uint nameOffset = BinaryPrimitives.ReadUInt32LittleEndian(span[cursor..]);
            uint nextBlock = BinaryPrimitives.ReadUInt32LittleEndian(span[(cursor + 4)..]);
            directories.Add(new WsDirectory(this, parent, ReadName(names, nameOffset), nextBlock));
        }

        for (uint i = 0; i < fileCount; i++, cursor += 56)
        {
            ReadOnlySpan<byte> record = span[cursor..];
            uint nameOffset = BinaryPrimitives.ReadUInt32LittleEndian(record);
            var compression = (WsCompression)BinaryPrimitives.ReadUInt32LittleEndian(record[4..]);
            ulong writeTime = BinaryPrimitives.ReadUInt64LittleEndian(record[8..]);
            ulong uncompressedSize = BinaryPrimitives.ReadUInt64LittleEndian(record[16..]);
            ulong compressedSize = BinaryPrimitives.ReadUInt64LittleEndian(record[24..]);
            byte[] hash = record.Slice(32, 20).ToArray();

            files.Add(new WsFile(this, parent, ReadName(names, nameOffset), compression,
                writeTime, uncompressedSize, compressedSize, hash));
        }
    }

    private static string ReadName(ReadOnlySpan<byte> names, uint offset)
    {
        if (offset >= names.Length)
        {
            return "(unnamed)";
        }

        ReadOnlySpan<byte> tail = names[(int)offset..];
        int end = tail.IndexOf((byte)0);
        return Encoding.UTF8.GetString(end < 0 ? tail : tail[..end]);
    }

    private static string[] SplitPath(string path) =>
        path.Replace('\\', '/').Split('/', StringSplitOptions.RemoveEmptyEntries);

    private static byte[] Decompress(byte[] raw, WsCompression compression, ulong uncompressedSize,
        string what)
    {
        if (compression != WsCompression.ZLib && compression != WsCompression.Lzma &&
            raw.Length > 5 && raw[0] == 0x5D)
        {
            compression = WsCompression.Lzma;
        }

        return compression switch
        {
            WsCompression.ZLib => Inflate(raw, Sized(uncompressedSize, what)),
            WsCompression.Lzma => WsLzma.Decode(raw, Sized(uncompressedSize, what), what),
            _ => raw,
        };
    }

    private static byte[] Inflate(byte[] raw, int expected)
    {
        var output = new byte[expected];

        using var source = new MemoryStream(raw, writable: false);
        using var zlib = new ZLibStream(source, CompressionMode.Decompress);

        int total = 0;
        while (total < output.Length)
        {
            int read = zlib.Read(output, total, output.Length - total);
            if (read == 0)
            {
                break;
            }

            total += read;
        }

        return total == output.Length ? output : output[..total];
    }

    private static int Sized(ulong size, string what)
    {
        if (size > int.MaxValue)
        {
            throw new InvalidDataException(
                what + ": " + size + " bytes is larger than one array holds");
        }

        return (int)size;
    }

    private static WsBlock[] ReadBlockTable(byte[] file, string path)
    {
        string name = System.IO.Path.GetFileName(path);

        if (file.Length < 8 + HeaderSize)
        {
            throw new InvalidDataException(name + ": too short to be a KCAP container");
        }

        if (BinaryPrimitives.ReadUInt32BigEndian(file) != MagicKcap)
        {
            throw new InvalidDataException(name + ": missing KCAP signature");
        }

        uint version = BinaryPrimitives.ReadUInt32LittleEndian(file.AsSpan(4));
        if (version != 1)
        {
            throw new InvalidDataException(name + ": unsupported KCAP version " + version);
        }

        ulong tableOffset = BinaryPrimitives.ReadUInt64LittleEndian(
            file.AsSpan(8 + BlockTableOffsetField));
        uint blockCount = BinaryPrimitives.ReadUInt32LittleEndian(
            file.AsSpan(8 + BlockCountField));

        if (tableOffset + (ulong)blockCount * 16 > (ulong)file.Length)
        {
            throw new InvalidDataException(name + ": block table runs past the end of the file");
        }

        var blocks = new WsBlock[blockCount];
        var span = file.AsSpan((int)tableOffset);
        for (int i = 0; i < blocks.Length; i++)
        {
            blocks[i] = new WsBlock(
                BinaryPrimitives.ReadUInt64LittleEndian(span[(i * 16)..]),
                BinaryPrimitives.ReadUInt64LittleEndian(span[(i * 16 + 8)..]));
        }

        return blocks;
    }

    private static WsBlock[] ReadBlockTable(FileStream file, string path)
    {
        string name = System.IO.Path.GetFileName(path);

        var header = new byte[8 + HeaderSize];
        file.Seek(0, SeekOrigin.Begin);
        file.ReadExactly(header, 0, header.Length);

        if (BinaryPrimitives.ReadUInt32BigEndian(header) != MagicKcap)
        {
            throw new InvalidDataException(name + ": missing KCAP signature");
        }

        uint version = BinaryPrimitives.ReadUInt32LittleEndian(header.AsSpan(4));
        if (version != 1)
        {
            throw new InvalidDataException(name + ": unsupported KCAP version " + version);
        }

        ulong tableOffset = BinaryPrimitives.ReadUInt64LittleEndian(
            header.AsSpan(8 + BlockTableOffsetField));
        uint blockCount = BinaryPrimitives.ReadUInt32LittleEndian(
            header.AsSpan(8 + BlockCountField));

        var table = new byte[checked((int)blockCount * 16)];
        file.Seek((long)tableOffset, SeekOrigin.Begin);
        file.ReadExactly(table, 0, table.Length);

        var blocks = new WsBlock[blockCount];
        for (int i = 0; i < blocks.Length; i++)
        {
            blocks[i] = new WsBlock(
                BinaryPrimitives.ReadUInt64LittleEndian(table.AsSpan(i * 16)),
                BinaryPrimitives.ReadUInt64LittleEndian(table.AsSpan(i * 16 + 8)));
        }

        return blocks;
    }

    private static uint FindRootBlock(byte[] index, WsBlock[] blocks, string path)
    {
        foreach (WsBlock block in blocks)
        {
            if (block.Size < 16 || block.Offset + 16 > (ulong)index.Length)
            {
                continue;
            }

            var span = index.AsSpan((int)block.Offset);
            if (BinaryPrimitives.ReadUInt32BigEndian(span) == MagicAidx)
            {
                return BinaryPrimitives.ReadUInt32LittleEndian(span[12..]);
            }
        }

        throw new InvalidDataException(System.IO.Path.GetFileName(path) +
            ": no AIDX block, so there is no directory tree");
    }

    private static Dictionary<Sha1Key, uint> ReadAarcTable(FileStream file, WsBlock[] blocks,
        string path)
    {
        var probe = new byte[16];
        uint entryCount = 0;
        uint tableBlock = 0;
        bool found = false;

        foreach (WsBlock block in blocks)
        {
            if (block.Size < 16)
            {
                continue;
            }

            file.Seek((long)block.Offset, SeekOrigin.Begin);
            file.ReadExactly(probe, 0, probe.Length);

            if (BinaryPrimitives.ReadUInt32BigEndian(probe) != MagicAarc)
            {
                continue;
            }

            entryCount = BinaryPrimitives.ReadUInt32LittleEndian(probe.AsSpan(8));
            tableBlock = BinaryPrimitives.ReadUInt32LittleEndian(probe.AsSpan(12));
            found = true;
            break;
        }

        if (!found || tableBlock >= blocks.Length)
        {
            throw new InvalidDataException(System.IO.Path.GetFileName(path) +
                ": no AARC table, so no file can be located");
        }

        var table = new byte[checked((int)entryCount * 32)];
        file.Seek((long)blocks[tableBlock].Offset, SeekOrigin.Begin);
        file.ReadExactly(table, 0, table.Length);

        var map = new Dictionary<Sha1Key, uint>((int)entryCount);
        for (int i = 0; i < entryCount; i++)
        {
            var entry = table.AsSpan(i * 32);
            map[new Sha1Key(entry.Slice(4, 20))] = BinaryPrimitives.ReadUInt32LittleEndian(entry);
        }

        return map;
    }

    private readonly struct Sha1Key : IEquatable<Sha1Key>
    {
        private readonly ulong _a;
        private readonly ulong _b;
        private readonly uint _c;

        public Sha1Key(ReadOnlySpan<byte> hash)
        {
            _a = BinaryPrimitives.ReadUInt64LittleEndian(hash);
            _b = BinaryPrimitives.ReadUInt64LittleEndian(hash[8..]);
            _c = BinaryPrimitives.ReadUInt32LittleEndian(hash[16..]);
        }

        public bool Equals(Sha1Key other) => _a == other._a && _b == other._b && _c == other._c;

        public override bool Equals(object? obj) => obj is Sha1Key other && Equals(other);

        public override int GetHashCode() => HashCode.Combine(_a, _b, _c);
    }
}
