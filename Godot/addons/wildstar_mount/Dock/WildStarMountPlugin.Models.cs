#if TOOLS
using System;
using Godot;
using WildStar.Archive;

namespace WildStar.Editor;

public partial class WildStarMountPlugin
{
    private M3SceneLoader? _modelLoader;

    private static bool IsModel(string name) =>
        name.EndsWith(".m3", StringComparison.OrdinalIgnoreCase);

    private void InstallModelLoader()
    {
        M3SceneLoader.SetResolver(ReadModelBytes);
        WildStar.Model.M3TextureCache.SetResolver(ReadModelBytes);
        _modelLoader = new M3SceneLoader();
        ResourceLoader.AddResourceFormatLoader(_modelLoader);
    }

    private void RemoveModelLoader()
    {
        if (_modelLoader is not null)
        {
            ResourceLoader.RemoveResourceFormatLoader(_modelLoader);
            _modelLoader = null;
        }

        M3SceneLoader.SetResolver(null);
        WildStar.Model.M3TextureCache.SetResolver(null);
    }

    private byte[]? ReadModelBytes(string path)
    {
        if (_filesystem is null)
        {
            return null;
        }

        if (!_filesystem.TryGetFile(path, out WsFile file))
        {
            bool found = false;
            foreach (WsArchive archive in _filesystem.Archives)
            {
                if (_filesystem.TryGetFile(archive.Name + "://" + path, out file))
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                return null;
            }
        }

        try
        {
            return file.ReadAllBytes();
        }
        catch (Exception exception)
        {
            GD.PushError("[wildstar_mount] " + path + ": " + exception.Message);
            return null;
        }
    }

    private void PlaceModel(WsFile file)
    {
        Node? scene = EditorInterface.Singleton.GetEditedSceneRoot();
        if (scene is null)
        {
            GD.PushWarning("[wildstar_mount] open a scene first, then drag in " +
                file.QualifiedPath);
            return;
        }

        var packed = ResourceLoader.Load<PackedScene>(
            file.QualifiedPath, "PackedScene", ResourceLoader.CacheMode.Ignore);

        if (packed is null)
        {
            return;
        }

        Node instance = packed.Instantiate();
        scene.AddChild(instance);
        instance.Owner = scene;

        EditorInterface.Singleton.GetSelection().Clear();
        EditorInterface.Singleton.GetSelection().AddNode(instance);
    }
}
#endif
