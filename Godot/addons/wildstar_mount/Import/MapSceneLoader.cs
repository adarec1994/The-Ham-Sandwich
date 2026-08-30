#if TOOLS
using System;
using Godot;
using WildStar.Area;

namespace WildStar.Editor;

[Tool]
public partial class MapSceneLoader : ResourceFormatLoader
{
    public const string Extension = ".tscn";

    public const float StreamRadius = 2048.0f;

    public static bool Recognises(string path) => TryMapName(path, out _);

    public static string ScenePathFor(string mapDirectoryQualifiedPath, string mapName) =>
        mapDirectoryQualifiedPath.TrimEnd('/') + "/" + mapName + Extension;

    public static bool TryMapName(string path, out string mapName)
    {
        mapName = string.Empty;
        if (!path.EndsWith(Extension, StringComparison.OrdinalIgnoreCase) ||
            !WsScenePath.IsServable(path))
        {
            return false;
        }

        string archivePath = WsScenePath.ToArchivePath(path);
        int separator = archivePath.IndexOf("://", StringComparison.Ordinal);
        string rest = (separator > 0 ? archivePath[(separator + 3)..] : archivePath).Replace('\\', '/');

        string[] parts = rest.Split('/');
        if (parts.Length != 3 || !parts[0].Equals("Map", StringComparison.OrdinalIgnoreCase))
        {
            return false;
        }

        string stem = parts[2][..^Extension.Length];
        if (stem.Length == 0 || !stem.Equals(parts[1], StringComparison.OrdinalIgnoreCase))
        {
            return false;
        }

        mapName = parts[1];
        return true;
    }

    public static void OpenInEditor(string archivePath)
    {
        string alias = WsScenePath.ToAlias(archivePath);
        EditorInterface editor = EditorInterface.Singleton;

        if (!ResourceLoader.Exists(alias) && WildStarMountPlugin.Instance?.EnsureLoaders() != true)
        {
            GD.PushError("[wildstar_mount] cannot open " + archivePath + ": the map scene loader " +
                         "is not registered — disable and re-enable the WildStar Mount plugin, " +
                         "or restart the editor");
            return;
        }

        bool alreadyOpen = Array.IndexOf(editor.GetOpenScenes(), alias) >= 0;
        bool placeholder = !alreadyOpen && editor.GetEditedSceneRoot() is null;

        GD.Print("[wildstar_mount] " + (alreadyOpen ? "switching to " : "opening ") + archivePath +
                 (alreadyOpen ? string.Empty : " — building the whole map, this can take a while"));

        AreaViewState.BeginOpen(alias);
        editor.OpenSceneFromPath(alias);
        AreaViewState.EndOpen(alias);

        if (placeholder && Array.IndexOf(editor.GetOpenScenes(), alias) >= 0)
        {
            AreaViewState.BeginOpen(alias);
            editor.ReloadSceneFromPath(alias);
            AreaViewState.EndOpen(alias);
        }

        editor.SetMainScreenEditor("3D");
    }

    public override string[] _GetRecognizedExtensions() => new[] { "tscn" };

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
        if (!TryMapName(path, out string mapName))
        {
            return Variant.CreateFrom((int)Error.FileUnrecognized);
        }

        bool opensAsScene = WsScenePath.IsAlias(path);
        MapRoot root = MapSceneBuilder.BuildStreaming(
            mapName, StreamRadius, message => GD.Print("[wildstar_mount]   " + message));

        foreach (Node child in root.GetChildren())
        {
            AreaSceneBuilder.Own(child, root);
        }

        if (opensAsScene)
        {
            AreaViewState.Prime(path, root);
        }

        var scene = new PackedScene();
        Error packed = scene.Pack(root);
        root.Free();

        if (packed != Error.Ok)
        {
            GD.PushWarning("[wildstar_mount] could not pack the " + mapName + " map scene");
            return Variant.CreateFrom((int)packed);
        }

        scene.ResourceName = mapName;
        if (!opensAsScene)
        {
            scene.ResourcePath = path;
        }

        return scene;
    }
}
#endif
