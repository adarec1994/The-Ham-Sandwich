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

    private static void RepackNormal(byte[] rgba)
    {
        for (int i = 0; i < rgba.Length; i += 4)
        {
            float nx = rgba[i + 3] / 255.0f * 2.0f - 1.0f;
            float ny = rgba[i + 1] / 255.0f * 2.0f - 1.0f;
            float nz = MathF.Sqrt(Math.Clamp(1.0f - nx * nx - ny * ny, 0.0f, 1.0f));

            rgba[i] = (byte)Math.Clamp((nx * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f);
            rgba[i + 1] = (byte)Math.Clamp((ny * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f);
            rgba[i + 2] = (byte)Math.Clamp((nz * 0.5f + 0.5f) * 255.0f, 0.0f, 255.0f);
            rgba[i + 3] = 255;
        }
    }

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

        if (tex.PixelMode == 1)
        {
            foreach (byte[] level in tex.Levels)
            {
                RepackNormal(level);
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
