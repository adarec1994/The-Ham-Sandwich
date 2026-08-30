using System;

namespace WildStar.Archive;

public sealed class WsFile
{
    internal WsFile(WsArchive archive, WsDirectory parent, string name, WsCompression compression,
        ulong writeTime, ulong uncompressedSize, ulong compressedSize, byte[] hash)
    {
        Archive = archive;
        Parent = parent;
        Name = name;
        Compression = compression;
        WriteTime = writeTime;
        UncompressedSize = uncompressedSize;
        CompressedSize = compressedSize;
        Hash = hash;
    }

    public WsArchive Archive { get; }

    public WsDirectory Parent { get; }

    public string Name { get; }

    public WsCompression Compression { get; }

    public ulong WriteTime { get; }

    public ulong UncompressedSize { get; }

    public ulong CompressedSize { get; }

    public byte[] Hash { get; }

    public string Path => Parent.Path.Length == 0 ? Name : Parent.Path + "/" + Name;

    public string QualifiedPath => Archive.Name + "://" + Path;

    public DateTime WriteTimeUtc =>
        WriteTime == 0 ? DateTime.MinValue : DateTime.FromFileTimeUtc((long)WriteTime);

    public byte[] ReadAllBytes() => Archive.ReadFile(this);

    public byte[] ReadPrefix(int maxBytes) => Archive.ReadFilePrefix(this, maxBytes);

    public byte[] ReadRawBlock() => Archive.ReadRawBlock(this);
}
