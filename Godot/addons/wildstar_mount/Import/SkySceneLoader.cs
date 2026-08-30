#if TOOLS
using System;
using Godot;
using WildStar.Sky;

namespace WildStar.Editor;

[Tool]
public partial class SkySceneLoader : ResourceFormatLoader
{
    private static Func<string, byte[]?>? _resolver;

    public static void SetResolver(Func<string, byte[]?>? resolver) => _resolver = resolver;

    public static bool Recognises(string path) =>
        path.EndsWith(".sky", StringComparison.OrdinalIgnoreCase) &&
        path.Contains("://", StringComparison.Ordinal) &&
        !path.StartsWith("res://", StringComparison.OrdinalIgnoreCase) &&
        !path.StartsWith("user://", StringComparison.OrdinalIgnoreCase);

    public override string[] _GetRecognizedExtensions() => new[] { "sky" };

    public override bool _RecognizePath(string path, StringName type) => Recognises(path);

    public override bool _Exists(string path) => Recognises(path);

    public override bool _HandlesType(StringName type)
    {
        string name = type.ToString();
        return name is "PackedScene" or "Resource";
    }

    public override string _GetResourceType(string path) =>
        Recognises(path) ? "PackedScene" : string.Empty;

    public override Variant _Load(
        string path, string originalPath, bool useSubThreads, int cacheMode)
    {
        if (!Recognises(path))
        {
            return Variant.CreateFrom((int)Error.FileUnrecognized);
        }

        byte[]? bytes = _resolver?.Invoke(path);
        if (bytes is null)
        {
            GD.PushWarning("[wildstar_mount] sky not found: " + path);
            return Variant.CreateFrom((int)Error.FileNotFound);
        }

        if (!SkyFile.TryParse(bytes, out SkyFile sky, out string parseError))
        {
            GD.PushWarning("[wildstar_mount] " + path + ": " + parseError);
            return Variant.CreateFrom((int)Error.FileCorrupt);
        }

        string name = NameOf(path);
        SkyRoot root = SkySceneBuilder.Build(sky, bytes, name);
        var scene = new PackedScene();
        Error packed = scene.Pack(root);
        root.Free();

        if (packed != Error.Ok)
        {
            GD.PushWarning("[wildstar_mount] could not pack " + path);
            return Variant.CreateFrom((int)packed);
        }

        scene.ResourceName = name;
        scene.ResourcePath = path;
        return scene;
    }

    private static string NameOf(string path)
    {
        int slash = path.LastIndexOfAny(new[] { '/', '\\' });
        string leaf = slash >= 0 ? path[(slash + 1)..] : path;
        return leaf[..^4];
    }
}
#endif
