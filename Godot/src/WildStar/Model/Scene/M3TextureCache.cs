using System;
using System.Collections.Generic;
using Godot;
using WildStar.Texture;

namespace WildStar.Model;

public static class M3TextureCache
{
    private static Func<string, byte[]?>? _resolver;
    private static readonly Dictionary<string, ImageTexture?> Cache = new();

    public static void SetResolver(Func<string, byte[]?>? resolver)
    {
        _resolver = resolver;
        Cache.Clear();
    }

    public static void Clear() => Cache.Clear();

    public static ImageTexture? Get(string path)
    {
        if (path.Length == 0 || _resolver is null)
        {
            return null;
        }

        if (Cache.TryGetValue(path, out ImageTexture? cached))
        {
            return cached;
        }

        ImageTexture? texture = Build(path);
        Cache[path] = texture;
        return texture;
    }

    private static ImageTexture? Build(string path)
    {

        string normalised = path.Replace('\\', '/');
        byte[]? bytes = _resolver?.Invoke(normalised);
        if (bytes is null)
        {
            return null;
        }

        if (!TexFile.TryDecode(bytes, out TexFile tex, out string error))
        {
            GD.PushWarning("[wildstar_mount] " + path + ": " + error);
            return null;
        }

        if (tex.IsNormalMap)
        {
            foreach (byte[] level in tex.Levels)
            {
                TexFile.RepackNormal(level);
            }
        }

        int total = 0;
        foreach (byte[] level in tex.Levels)
        {
            total += level.Length;
        }

        var data = new byte[total];
        int at = 0;
        for (int mip = 0; mip < tex.Levels.Length; mip++)
        {
            byte[] level = tex.Levels[mip];
            Buffer.BlockCopy(level, 0, data, at, level.Length);
            at += level.Length;
        }

        Image image = Image.CreateFromData(tex.Width, tex.Height, true,
                                           Image.Format.Rgba8, data);
        return ImageTexture.CreateFromImage(image);
    }
}
