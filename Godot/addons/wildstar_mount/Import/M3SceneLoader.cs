#if TOOLS
using System;
using Godot;
using WildStar.Model;

namespace WildStar.Editor;

[Tool]
public partial class M3SceneLoader : ResourceFormatLoader
{
    private static Func<string, byte[]?>? _resolver;

    public static void SetResolver(Func<string, byte[]?>? resolver) => _resolver = resolver;

    public static bool Recognises(string path) =>
        path.EndsWith(".m3", StringComparison.OrdinalIgnoreCase) &&
        path.Contains("://", StringComparison.Ordinal) &&
        !path.StartsWith("res://", StringComparison.OrdinalIgnoreCase) &&
        !path.StartsWith("user://", StringComparison.OrdinalIgnoreCase);

    public override string[] _GetRecognizedExtensions() => new[] { "m3" };

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
            GD.PushWarning("[wildstar_mount] model not found: " + path);
            return Variant.CreateFrom((int)Error.FileNotFound);
        }

        if (!M3File.TryParse(bytes, out M3File model, out string parseError))
        {
            GD.PushWarning("[wildstar_mount] " + path + ": " + parseError);
            return Variant.CreateFrom((int)Error.FileCorrupt);
        }

        if (!M3MeshBuilder.TryBuild(model, out ArrayMesh mesh, out int[] surfaceGeosets,
                                    out int[] surfaceMaterials, out string buildError))
        {
            GD.PushWarning("[wildstar_mount] " + path + ": " + buildError);
            return Variant.CreateFrom((int)Error.FileCorrupt);
        }

        string name = NameOf(path);
        mesh.ResourceName = name;

        Node3D root = M3SceneBuilder.Build(model, mesh, name, bytes, surfaceGeosets, surfaceMaterials, path);
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
        return leaf[..^3];
    }
}
#endif
