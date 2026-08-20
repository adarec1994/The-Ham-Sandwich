using System;
using System.Collections.Generic;

namespace WildStar.Archive;

public sealed class WsDirectory
{
    private readonly uint _blockIndex;
    private List<WsDirectory>? _directories;
    private List<WsFile>? _files;

    internal WsDirectory(WsArchive archive, WsDirectory? parent, string name, uint blockIndex)
    {
        Archive = archive;
        Parent = parent;
        Name = name;
        _blockIndex = blockIndex;
    }

    public WsArchive Archive { get; }

    public WsDirectory? Parent { get; }

    public string Name { get; }

    public string Path =>
        Parent is null || Parent.Path.Length == 0 ? Name : Parent.Path + "/" + Name;

    public string QualifiedPath => Archive.Name + "://" + Path;

    public IReadOnlyList<WsDirectory> Directories
    {
        get
        {
            EnsureParsed();
            return _directories!;
        }
    }

    public IReadOnlyList<WsFile> Files
    {
        get
        {
            EnsureParsed();
            return _files!;
        }
    }

    public bool TryGetDirectory(string name, out WsDirectory directory)
    {
        foreach (WsDirectory child in Directories)
        {
            if (string.Equals(child.Name, name, StringComparison.OrdinalIgnoreCase))
            {
                directory = child;
                return true;
            }
        }

        directory = null!;
        return false;
    }

    public bool TryGetFile(string name, out WsFile file)
    {
        foreach (WsFile child in Files)
        {
            if (string.Equals(child.Name, name, StringComparison.OrdinalIgnoreCase))
            {
                file = child;
                return true;
            }
        }

        file = null!;
        return false;
    }

    public IEnumerable<WsFile> EnumerateFilesRecursive()
    {
        foreach (WsFile file in Files)
        {
            yield return file;
        }

        foreach (WsDirectory child in Directories)
        {
            foreach (WsFile file in child.EnumerateFilesRecursive())
            {
                yield return file;
            }
        }
    }

    private void EnsureParsed()
    {
        if (_directories is not null)
        {
            return;
        }

        lock (this)
        {
            if (_directories is not null)
            {
                return;
            }

            var directories = new List<WsDirectory>();
            var files = new List<WsFile>();
            Archive.ParseDirectoryBlock(this, _blockIndex, directories, files);

            directories.Sort(static (a, b) =>
                string.Compare(a.Name, b.Name, StringComparison.OrdinalIgnoreCase));
            files.Sort(static (a, b) =>
                string.Compare(a.Name, b.Name, StringComparison.OrdinalIgnoreCase));

            _files = files;
            _directories = directories;
        }
    }
}
