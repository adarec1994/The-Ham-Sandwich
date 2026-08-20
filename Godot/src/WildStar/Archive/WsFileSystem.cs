using System;
using System.Collections.Generic;
using System.IO;

namespace WildStar.Archive;

public sealed class WsFileSystem : IDisposable
{
    private readonly List<WsArchive> _archives;
    private readonly Dictionary<string, WsArchive> _byName;
    private bool _disposed;

    private WsFileSystem(string gameDirectory, List<WsArchive> archives, List<string> warnings)
    {
        GameDirectory = gameDirectory;
        _archives = archives;
        Warnings = warnings;

        _byName = new Dictionary<string, WsArchive>(StringComparer.OrdinalIgnoreCase);
        foreach (WsArchive archive in archives)
        {
            _byName[archive.Name] = archive;
        }
    }

    public string GameDirectory { get; }

    public IReadOnlyList<WsArchive> Archives => _archives;

    public IReadOnlyList<string> Warnings { get; }

    public static WsFileSystem Mount(string gameDirectory)
    {
        var archives = new List<WsArchive>();
        var warnings = new List<string>();

        if (!Directory.Exists(gameDirectory))
        {
            warnings.Add("No such directory: " + gameDirectory);
            return new WsFileSystem(gameDirectory, archives, warnings);
        }

        foreach (string indexPath in FindIndexFiles(gameDirectory, warnings))
        {
            try
            {
                archives.Add(WsArchive.Open(indexPath));
            }
            catch (Exception exception)
            {
                warnings.Add(Path.GetFileName(indexPath) + ": " + exception.Message);
            }
        }

        archives.Sort(static (a, b) =>
            string.Compare(a.Name, b.Name, StringComparison.OrdinalIgnoreCase));

        return new WsFileSystem(gameDirectory, archives, warnings);
    }

    public bool TryGetArchive(string name, out WsArchive archive) =>
        _byName.TryGetValue(name, out archive!);

    public bool TryGetFile(string qualifiedPath, out WsFile file)
    {
        file = null!;
        if (!TrySplitQualified(qualifiedPath, out string name, out string inner))
        {
            return false;
        }

        return _byName.TryGetValue(name, out WsArchive? archive) &&
               archive.TryGetFile(inner, out file);
    }

    public bool TryGetDirectory(string qualifiedPath, out WsDirectory directory)
    {
        directory = null!;
        if (!TrySplitQualified(qualifiedPath, out string name, out string inner))
        {
            return false;
        }

        return _byName.TryGetValue(name, out WsArchive? archive) &&
               archive.TryGetDirectory(inner, out directory);
    }

    public static bool TrySplitQualified(string qualifiedPath, out string archiveName,
        out string innerPath)
    {
        int separator = qualifiedPath.IndexOf("://", StringComparison.Ordinal);
        if (separator <= 0)
        {
            archiveName = string.Empty;
            innerPath = string.Empty;
            return false;
        }

        archiveName = qualifiedPath[..separator];
        innerPath = qualifiedPath[(separator + 3)..];
        return true;
    }

    public static bool HasArchives(string directory)
    {
        if (!Directory.Exists(directory))
        {
            return false;
        }

        foreach (string unused in FindIndexFiles(directory, null))
        {
            return true;
        }

        return false;
    }

    public static IEnumerable<string> ProbePaths(string? repositoryRoot = null)
    {
        if (repositoryRoot is not null)
        {
            yield return Path.Combine(repositoryRoot, "wildstar_path.cfg");
            yield return Path.Combine(repositoryRoot, "cmake-build-debug", "wildstar_path.cfg");
        }

        yield return @"C:\Program Files (x86)\NCSOFT\WildStar";
        yield return @"C:\Program Files (x86)\Steam\steamapps\common\WildStar";
        yield return @"C:\Program Files\NCSOFT\WildStar";
        yield return @"C:\Program Files\Steam\steamapps\common\WildStar";

        string home = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
        if (home.Length != 0)
        {
            yield return Path.Combine(home, "Documents", "WildStar");
            yield return Path.Combine(home, "OneDrive", "Documents", "WildStar");
        }
    }

    public static string? AutoDetect(string? repositoryRoot = null)
    {
        foreach (string candidate in ProbePaths(repositoryRoot))
        {
            string directory = candidate;

            if (candidate.EndsWith(".cfg", StringComparison.OrdinalIgnoreCase))
            {
                if (!File.Exists(candidate))
                {
                    continue;
                }

                try
                {
                    directory = File.ReadAllText(candidate).Trim();
                }
                catch (Exception)
                {
                    continue;
                }

                if (directory.Length == 0)
                {
                    continue;
                }
            }

            if (HasArchives(directory))
            {
                return directory;
            }
        }

        return null;
    }

    public void Dispose()
    {
        if (_disposed)
        {
            return;
        }

        _disposed = true;
        foreach (WsArchive archive in _archives)
        {
            archive.Dispose();
        }

        _archives.Clear();
        _byName.Clear();
    }

    private static IEnumerable<string> FindIndexFiles(string directory, List<string>? warnings)
    {
        IEnumerable<string> candidates;
        try
        {
            candidates = Directory.EnumerateFiles(directory, "*.index", SearchOption.AllDirectories);
        }
        catch (Exception exception)
        {
            warnings?.Add(directory + ": " + exception.Message);
            yield break;
        }

        foreach (string indexPath in candidates)
        {
            if (File.Exists(Path.ChangeExtension(indexPath, ".archive")))
            {
                yield return indexPath;
            }
        }
    }
}
