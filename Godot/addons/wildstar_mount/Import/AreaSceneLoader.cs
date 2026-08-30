#if TOOLS
using System;
using Godot;
using WildStar.Area;

namespace WildStar.Editor;

[Tool]
public partial class AreaSceneLoader : ResourceFormatLoader
{
    private static Func<string, byte[]?>? _resolver;

    public static void SetResolver(Func<string, byte[]?>? resolver) => _resolver = resolver;

    public static bool Recognises(string path) =>
        path.EndsWith(".area", StringComparison.OrdinalIgnoreCase) &&
        WsScenePath.IsServable(path);

    public static void OpenInEditor(string archivePath)
    {
        string alias = WsScenePath.ToAlias(archivePath);
        EditorInterface editor = EditorInterface.Singleton;

        if (!ResourceLoader.Exists(alias) && WildStarMountPlugin.Instance?.EnsureLoaders() != true)
        {
            GD.PushError("[wildstar_mount] cannot open " + archivePath + ": the .area scene " +
                         "loader is not registered — disable and re-enable the WildStar Mount " +
                         "plugin, or restart the editor");
            return;
        }

        bool alreadyOpen = Array.IndexOf(editor.GetOpenScenes(), alias) >= 0;

        bool placeholder = !alreadyOpen && editor.GetEditedSceneRoot() is null;

        GD.Print("[wildstar_mount] " + (alreadyOpen ? "switching to " : "opening ") + archivePath +
                 (alreadyOpen ? string.Empty : " as a scene"));

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

    public override string[] _GetRecognizedExtensions() => new[] { "area" };

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

        string archivePath = WsScenePath.ToArchivePath(path);
        bool opensAsScene = WsScenePath.IsAlias(path);

        byte[]? bytes = _resolver?.Invoke(archivePath);
        if (bytes is null)
        {
            GD.PushWarning("[wildstar_mount] area not found: " + archivePath);
            return Variant.CreateFrom((int)Error.FileNotFound);
        }

        if (!AreaFile.TryParse(bytes, out AreaFile area, out string parseError))
        {
            GD.PushWarning("[wildstar_mount] " + archivePath + ": " + parseError);
            return Variant.CreateFrom((int)Error.FileCorrupt);
        }

        if (!AreaTileCoord.TryParse(archivePath, out string mapName, out AreaTileCoord tile))
        {
            GD.PushWarning("[wildstar_mount] " + archivePath + ": file name is not <map>.<zz><xx>.area");
            return Variant.CreateFrom((int)Error.FileUnrecognized);
        }

        string name = NameOf(archivePath);
        AreaRoot root = AreaSceneBuilder.Build(area, mapName, tile, name, true);
        WarnIfEmpty(archivePath, area, tile, root);
        if (opensAsScene)
        {
            AreaViewState.Prime(path, root, tile);
        }

        var scene = new PackedScene();
        Error packed = scene.Pack(root);
        root.Free();

        if (packed != Error.Ok)
        {
            GD.PushWarning("[wildstar_mount] could not pack " + archivePath);
            return Variant.CreateFrom((int)packed);
        }

        scene.ResourceName = name;
        if (!opensAsScene)
        {
            scene.ResourcePath = path;
        }

        return scene;
    }

    private static void WarnIfEmpty(string archivePath, AreaFile area, AreaTileCoord tile, AreaRoot root)
    {
        if (root.GetNodeOrNull<Node3D>(AreaRoot.ChunksNode) is not Node3D chunks ||
            chunks.GetChildCount() > 0)
        {
            return;
        }

        int present = 0;
        uint layers = 0;
        foreach (AreaChunk chunk in area.PresentChunks())
        {
            present++;
            layers |= chunk.LayerFlags;
        }

        var named = new System.Collections.Generic.List<int>();
        for (int i = 0; i < AreaLayerTable.Count; i++)
        {
            if ((layers & (1u << i)) != 0)
            {
                named.Add(i);
            }
        }

        int heightsLayer = tile.Low ? AreaLayerTable.LowHeights : AreaLayerTable.Heights;
        GD.PushWarning($"[wildstar_mount] {archivePath}: no terrain to build — none of the " +
                       $"{present} chunks has a heights layer ({heightsLayer}); layers present: " +
                       $"[{string.Join(",", named)}]. The scene opens empty.");
    }

    private static string NameOf(string path)
    {
        int slash = path.LastIndexOfAny(new[] { '/', '\\' });
        string leaf = slash >= 0 ? path[(slash + 1)..] : path;
        return leaf[..^5];
    }
}
#endif
