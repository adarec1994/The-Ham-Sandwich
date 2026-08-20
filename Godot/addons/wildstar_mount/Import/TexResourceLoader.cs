#if TOOLS
using System;
using Godot;
using WildStar.Texture;

namespace WildStar.Editor;

[Tool]
public partial class TexResourceLoader : ResourceFormatLoader
{
    private static Func<string, byte[]?>? _resolver;

    public static void SetResolver(Func<string, byte[]?>? resolver) => _resolver = resolver;

    public static bool Recognises(string path) =>
        path.EndsWith(".tex", StringComparison.OrdinalIgnoreCase) &&
        path.Contains("://", StringComparison.Ordinal) &&
        !path.StartsWith("res://", StringComparison.OrdinalIgnoreCase) &&
        !path.StartsWith("user://", StringComparison.OrdinalIgnoreCase);

    public static Image? DecodeImage(byte[] bytes, out string error)
    {
        if (!TexFile.TryDecode(bytes, out TexFile tex, out error))
        {
            return null;
        }

        if (tex.Levels.Length == 0 || tex.Levels[0].Length != tex.Width * tex.Height * 4)
        {
            error = "unexpected pixel data";
            return null;
        }

        byte[] pixels = tex.Levels[0];
        if (tex.IsNormalMap)
        {
            TexFile.RepackNormal(pixels);
        }

        error = string.Empty;
        return Image.CreateFromData(tex.Width, tex.Height, false, Image.Format.Rgba8, pixels);
    }

    public static ImageTexture? Decode(byte[] bytes, out string error)
    {
        Image? image = DecodeImage(bytes, out error);
        if (image is null)
        {
            return null;
        }

        image.GenerateMipmaps();
        return ImageTexture.CreateFromImage(image);
    }

    public static string NameOf(string path)
    {
        int slash = path.LastIndexOfAny(new[] { '/', '\\' });
        string leaf = slash >= 0 ? path[(slash + 1)..] : path;
        int dot = leaf.LastIndexOf('.');
        return dot > 0 ? leaf[..dot] : leaf;
    }

    public override string[] _GetRecognizedExtensions() => new[] { "tex" };

    public override bool _RecognizePath(string path, StringName type) => Recognises(path);

    public override bool _Exists(string path) => Recognises(path);

    public override bool _HandlesType(StringName type)
    {
        string name = type.ToString();
        return name is "Texture2D" or "ImageTexture" or "Resource";
    }

    public override string _GetResourceType(string path) =>
        Recognises(path) ? "Texture2D" : string.Empty;

    public override Variant _Load(
        string path, string originalPath, bool useSubThreads, int cacheMode)
    {
        if (!Recognises(path))
            return Variant.CreateFrom((int)Error.FileUnrecognized);

        byte[]? bytes = _resolver?.Invoke(path);
        if (bytes is null)
        {
            GD.PushWarning("[wildstar_mount] texture not found: " + path);
            return Variant.CreateFrom((int)Error.FileNotFound);
        }

        ImageTexture? texture = Decode(bytes, out string error);
        if (texture is null)
        {
            GD.PushWarning("[wildstar_mount] " + path + ": " + error);
            return Variant.CreateFrom((int)Error.FileCorrupt);
        }

        texture.ResourceName = NameOf(path);
        texture.ResourcePath = path;
        return texture;
    }
}
#endif
