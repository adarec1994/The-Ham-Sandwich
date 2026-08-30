using System;
using System.Collections.Generic;
using Godot;
using WildStar.Texture;

namespace WildStar.Model;

public enum M3TextureAlpha
{
    Keep,
    Opaque,
    Invert,
}

public static class M3TextureCache
{
    private static Func<string, byte[]?>? _resolver;
    private static Func<string, string[]>? _group;
    private static readonly Dictionary<string, ImageTexture?> Cache = new();

    public static string PartHint { get; set; } = string.Empty;

    public static void SetResolver(Func<string, byte[]?>? resolver)
    {
        _resolver = resolver;
        Cache.Clear();
    }

    public static void SetGroupResolver(Func<string, string[]>? group)
    {
        _group = group;
        Cache.Clear();
    }

    public static void Clear()
    {
        Cache.Clear();
    }

    public readonly struct CpuTexture
    {
        public CpuTexture(int width, int height, bool mipmaps, Image.Format format, byte[] data)
        {
            Width = width;
            Height = height;
            Mipmaps = mipmaps;
            Format = format;
            Data = data;
        }

        public int Width { get; }

        public int Height { get; }

        public bool Mipmaps { get; }

        public Image.Format Format { get; }

        public byte[] Data { get; }
    }

    private static readonly System.Collections.Concurrent.ConcurrentDictionary<string, CpuTexture?> Prepared = new();

    public static void Prewarm(System.Collections.Generic.IReadOnlyCollection<string> paths,
                               M3TextureAlpha alpha = M3TextureAlpha.Keep)
    {
        if (_resolver is null)
        {
            return;
        }

        string hint = PartHint;
        System.Threading.Tasks.Parallel.ForEach(paths, path =>
        {
            if (path.Length == 0)
            {
                return;
            }

            string key = path + "|" + hint + "|" + alpha;
            if (Cache.ContainsKey(key) || Prepared.ContainsKey(key))
            {
                return;
            }

            Prepared[key] = BuildCpu(path, alpha);
        });
    }

    public static ImageTexture? Get(string path, M3TextureAlpha alpha = M3TextureAlpha.Keep)
    {
        if (path.Length == 0 || _resolver is null)
        {
            return null;
        }

        string key = path + "|" + PartHint + "|" + alpha;
        if (Cache.TryGetValue(key, out ImageTexture? cached))
        {
            return cached;
        }

        ImageTexture? texture;
        if (Prepared.TryRemove(key, out CpuTexture? prepared))
        {
            texture = Wrap(prepared);
        }
        else
        {
            texture = Wrap(BuildCpu(path, alpha));
        }

        Cache[key] = texture;
        return texture;
    }

    private static ImageTexture? Wrap(CpuTexture? cpu)
    {
        if (cpu is not CpuTexture ready)
        {
            return null;
        }

        try
        {
            Image image = Image.CreateFromData(ready.Width, ready.Height, ready.Mipmaps,
                                               ready.Format, ready.Data);
            return image is null ? null : ImageTexture.CreateFromImage(image);
        }
        catch (Exception)
        {
            return null;
        }
    }

    private static CpuTexture? BuildCpu(string path, M3TextureAlpha alpha)
    {
        string normalised = path.Replace('\\', '/');
        byte[]? bytes = _resolver?.Invoke(normalised);

        if (bytes is null)
        {
            string? variant = PickVariant(normalised);
            if (variant is not null)
            {
                bytes = _resolver?.Invoke(variant);
            }
        }

        if (bytes is null)
        {
            return null;
        }

        if (alpha == M3TextureAlpha.Keep &&
            TexFile.TryGetRawDxt(bytes, out int rawW, out int rawH, out int rawFormat,
                                 out bool rawMips, out byte[] raw))
        {
            bool normalMap = bytes.Length > 0x24 &&
                             System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(
                                 bytes.AsSpan(0x24)) == TexFile.NormalPixelMode;
            if (!normalMap)
            {
                Image.Format gpu = rawFormat switch
                {
                    TexFile.FormatDxt3 => Image.Format.Dxt3,
                    TexFile.FormatDxt5 => Image.Format.Dxt5,
                    _ => Image.Format.Dxt1,
                };
                return new CpuTexture(rawW, rawH, rawMips, gpu, raw);
            }
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

        switch (alpha)
        {
            case M3TextureAlpha.Opaque:
                for (int i = 3; i < data.Length; i += 4)
                {
                    data[i] = 255;
                }

                break;
            case M3TextureAlpha.Invert:
                for (int i = 3; i < data.Length; i += 4)
                {
                    data[i] = (byte)(255 - data[i]);
                }

                break;
        }

        return new CpuTexture(tex.Width, tex.Height, true, Image.Format.Rgba8, data);
    }

    private static string? PickVariant(string normalised)
    {
        if (_group is null || !normalised.EndsWith(".tex", StringComparison.OrdinalIgnoreCase))
        {
            return null;
        }

        string[] candidates = _group(normalised[..^4]);
        if (candidates.Length == 0)
        {
            return null;
        }

        string? best = null;
        int bestScore = -1;

        foreach (string candidate in candidates)
        {
            int score = Score(candidate);
            if (score > bestScore)
            {
                bestScore = score;
                best = candidate;
            }
        }

        return best;
    }

    private static int Score(string candidate)
    {
        string part = PartOf(candidate);
        if (part.Length == 0 || PartHint.Length == 0)
        {
            return 0;
        }

        int underscore = part.LastIndexOf('_');
        string leaf = underscore >= 0 ? part[(underscore + 1)..] : part;

        if (leaf.Length >= 3 && PartHint.Contains(leaf, StringComparison.OrdinalIgnoreCase))
        {
            return 2;
        }

        return PartHint.Contains(part, StringComparison.OrdinalIgnoreCase) ? 1 : 0;
    }

    private static string PartOf(string path)
    {
        if (!path.EndsWith(".tex", StringComparison.OrdinalIgnoreCase))
        {
            return string.Empty;
        }

        string noExt = path[..^4];
        int dot = noExt.LastIndexOf('.');
        int slash = noExt.LastIndexOf('/');
        return dot > slash ? noExt[(dot + 1)..] : string.Empty;
    }
}
