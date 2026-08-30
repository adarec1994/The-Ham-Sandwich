#if TOOLS
using System;

namespace WildStar.Editor;

public static class WsScenePath
{
    public const string AliasRoot = "res://.wildstar/";

    public static bool IsArchivePath(string path) =>
        path.Contains("://", StringComparison.Ordinal) &&
        !path.StartsWith("res://", StringComparison.OrdinalIgnoreCase) &&
        !path.StartsWith("user://", StringComparison.OrdinalIgnoreCase);

    public static bool IsAlias(string path) =>
        path.StartsWith(AliasRoot, StringComparison.OrdinalIgnoreCase);

    public static string ToAlias(string archivePath)
    {
        int separator = archivePath.IndexOf("://", StringComparison.Ordinal);
        if (separator <= 0)
        {
            throw new ArgumentException("not an archive path: " + archivePath, nameof(archivePath));
        }

        return AliasRoot + archivePath[..separator] + "/" +
               archivePath[(separator + 3)..].Replace('\\', '/');
    }

    public static bool TryFromAlias(string path, out string archivePath)
    {
        archivePath = string.Empty;
        if (!IsAlias(path))
        {
            return false;
        }

        string rest = path[AliasRoot.Length..];
        int slash = rest.IndexOf('/');
        if (slash <= 0 || slash == rest.Length - 1)
        {
            return false;
        }

        archivePath = rest[..slash] + "://" + rest[(slash + 1)..];
        return true;
    }

    public static string ToArchivePath(string path) =>
        TryFromAlias(path, out string archivePath) ? archivePath : path;

    public static bool IsServable(string path) =>
        IsArchivePath(path) || TryFromAlias(path, out _);
}
#endif
