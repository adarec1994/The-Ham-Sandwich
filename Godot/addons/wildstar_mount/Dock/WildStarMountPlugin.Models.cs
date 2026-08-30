#if TOOLS
using System;
using System.Threading.Tasks;
using Godot;
using WildStar.Archive;

namespace WildStar.Editor;

public partial class WildStarMountPlugin
{
    private M3SceneLoader? _modelLoader;
    private TexResourceLoader? _texLoader;
    private SkySceneLoader? _skyLoader;
    private AreaSceneLoader? _areaLoader;
    private MapSceneLoader? _mapLoader;

    private static bool IsModel(string name) =>
        name.EndsWith(".m3", StringComparison.OrdinalIgnoreCase);

    private static bool IsSky(string name) =>
        name.EndsWith(".sky", StringComparison.OrdinalIgnoreCase);

    private static bool IsArea(string name) =>
        name.EndsWith(".area", StringComparison.OrdinalIgnoreCase);

    private void OpenMap(string scenePath)
    {
        if (_mapLoader is null)
        {
            GD.PushWarning("[wildstar_mount] the map loader is not installed");
            return;
        }

        MapSceneLoader.OpenInEditor(scenePath);
    }

    private void OpenArea(WsFile file)
    {
        if (_areaLoader is null)
        {
            GD.PushWarning("[wildstar_mount] the area loader is not installed");
            return;
        }

        AreaSceneLoader.OpenInEditor(file.QualifiedPath);
    }

    private static bool IsMapDirectory(WsDirectory directory) =>
        directory.Path.StartsWith("Map/", StringComparison.OrdinalIgnoreCase) &&
        directory.Path.IndexOf('/', 4) < 0;

    private void PlaceMap(WsDirectory directory)
    {
        Node? scene = EditorInterface.Singleton.GetEditedSceneRoot();
        if (scene is null)
        {
            GD.PushWarning("[wildstar_mount] open a scene first, then load " + directory.QualifiedPath);
            return;
        }

        string mapName = directory.Name;
        GD.Print("[wildstar_mount] loading map " + mapName +
                 " (every detail tile — a continent can take a while and several GB)");
        WildStar.Area.MapRoot map = WildStar.Area.MapSceneBuilder.Build(
            mapName, 0, 0, -1, true, message => GD.Print("[wildstar_mount]   " + message));
        scene.AddChild(map);
        WildStar.Area.AreaSceneBuilder.Own(map, scene);
        GD.Print($"[wildstar_mount] {mapName}: {map.DetailTiles} tiles, sky {map.SkyId}");
        EditorInterface.Singleton.GetSelection().Clear();
        EditorInterface.Singleton.GetSelection().AddNode(map);
    }

    private void PlaceMapAround(WsFile file)
    {
        if (!WildStar.Area.AreaTileCoord.TryParse(file.Path, out string mapName, out WildStar.Area.AreaTileCoord coord))
        {
            GD.PushWarning("[wildstar_mount] " + file.QualifiedPath + " is not a map tile");
            return;
        }

        Node? scene = EditorInterface.Singleton.GetEditedSceneRoot();
        if (scene is null)
        {
            GD.PushWarning("[wildstar_mount] open a scene first, then load " + file.QualifiedPath);
            return;
        }

        int focusX = coord.Low ? coord.X * 8 + 4 : coord.X;
        int focusZ = coord.Low ? coord.Z * 8 + 4 : coord.Z;
        GD.Print($"[wildstar_mount] loading map {mapName} around tile ({focusX},{focusZ}), radius {WildStar.Area.MapSceneBuilder.FallbackRadius}");
        WildStar.Area.MapRoot map = WildStar.Area.MapSceneBuilder.Build(
            mapName, focusX, focusZ, WildStar.Area.MapSceneBuilder.FallbackRadius, false,
            message => GD.Print("[wildstar_mount]   " + message));
        scene.AddChild(map);
        WildStar.Area.AreaSceneBuilder.Own(map, scene);
        GD.Print($"[wildstar_mount] {mapName}: {map.DetailTiles} tiles, sky {map.SkyId}");
        EditorInterface.Singleton.GetSelection().Clear();
        EditorInterface.Singleton.GetSelection().AddNode(map);
    }

    private const string PreviewSkyName = "WsPreviewSky";

    private string _previewSkyFailed = string.Empty;

    private void EnsurePreviewSky()
    {
        if (_filesystem is null || _filesystem.Archives.Count == 0)
        {
            return;
        }

        if (EditorInterface.Singleton.GetEditedSceneRoot() is not WildStar.Model.M3ModelRoot root ||
            root.HasNode(PreviewSkyName))
        {
            return;
        }

        if (root.FindChildren("*", "WorldEnvironment", true, false).Count > 0 ||
            root.FindChildren("*", "DirectionalLight3D", true, false).Count > 0)
        {
            return;
        }

        string path = ProjectSettings.GetSetting(DefaultSkySetting, DefaultSkyPath).AsString();
        if (path.Length == 0 || string.Equals(path, _previewSkyFailed, StringComparison.OrdinalIgnoreCase))
        {
            return;
        }

        WildStar.Sky.SkyRoot? sky = WildStar.Area.AreaSceneBuilder.BuildSky(path);
        if (sky is null)
        {
            _previewSkyFailed = path;
            return;
        }

        sky.Name = PreviewSkyName;
        root.AddChild(sky);
        GD.Print($"[wildstar_mount] {root.Name}: preview sky {path} attached ({DefaultSkySetting})");
    }

    private static bool IsTexture(string name) =>
        name.EndsWith(".tex", StringComparison.OrdinalIgnoreCase);

    private static bool IsTable(string name) =>
        name.EndsWith(".tbl", StringComparison.OrdinalIgnoreCase);

    private void OpenTable(WsFile file)
    {
        byte[] bytes;
        try
        {
            bytes = file.ReadAllBytes();
        }
        catch (Exception exception)
        {
            GD.PushError("[wildstar_mount] " + file.QualifiedPath + ": " + exception.Message);
            return;
        }

        if (!WildStar.GameTable.TblReader.TryParse(bytes, out var table, out string error))
        {
            GD.PushWarning("[wildstar_mount] " + file.QualifiedPath + ": " + error);
            return;
        }

        GD.Print($"[wildstar_mount] {file.QualifiedPath} — {table.Name}: " +
                 $"{table.RecordCount} records x {table.FieldCount} fields");

        TblViewer.Open(table, file.QualifiedPath);
    }

    internal const string LoaderProbe = "res://.wildstar/__probe__/__probe__.area";

    internal bool EnsureLoaders()
    {
        if (ResourceLoader.Exists(LoaderProbe))
        {
            return true;
        }

        GD.Print("[wildstar_mount] the resource loaders are no longer registered (a .NET " +
                 "assembly reload does this) — re-registering them; any \"loader_count\" " +
                 "errors right below are the stale ones being cleared");
        RemoveModelLoader();
        InstallModelLoader();
        return ResourceLoader.Exists(LoaderProbe);
    }

    internal void DropLoadersForTest()
    {
        if (_modelLoader is not null)
        {
            ResourceLoader.RemoveResourceFormatLoader(_modelLoader);
        }

        if (_texLoader is not null)
        {
            ResourceLoader.RemoveResourceFormatLoader(_texLoader);
        }

        if (_skyLoader is not null)
        {
            ResourceLoader.RemoveResourceFormatLoader(_skyLoader);
        }

        if (_areaLoader is not null)
        {
            ResourceLoader.RemoveResourceFormatLoader(_areaLoader);
        }

        if (_mapLoader is not null)
        {
            ResourceLoader.RemoveResourceFormatLoader(_mapLoader);
        }
    }

    private void InstallModelLoader()
    {
        M3SceneLoader.SetResolver(ReadModelBytes);
        TexResourceLoader.SetResolver(ReadModelBytes);
        WildStar.Model.M3TextureCache.SetResolver(ReadModelBytes);
        WildStar.Model.M3TextureCache.SetGroupResolver(FindTextureVariants);
        WildStar.Model.M3SceneBuilder.SetFileSystem(() => _filesystem);
        SkySceneLoader.SetResolver(ReadModelBytes);
        WildStar.Sky.SkySceneBuilder.SetResolver(ReadModelBytes);
        AreaSceneLoader.SetResolver(ReadModelBytes);
        WildStar.Area.AreaTables.SetResolver(ReadModelBytes);
        WildStar.Area.AreaTables.SetFileSystem(() => _filesystem);
        _modelLoader = new M3SceneLoader();
        _texLoader = new TexResourceLoader();
        _skyLoader = new SkySceneLoader();
        _areaLoader = new AreaSceneLoader();
        _mapLoader = new MapSceneLoader();
        ResourceLoader.AddResourceFormatLoader(_modelLoader);
        ResourceLoader.AddResourceFormatLoader(_texLoader);
        ResourceLoader.AddResourceFormatLoader(_skyLoader);
        ResourceLoader.AddResourceFormatLoader(_areaLoader);
        ResourceLoader.AddResourceFormatLoader(_mapLoader);
    }

    private void RemoveModelLoader()
    {
        if (_modelLoader is not null)
        {
            ResourceLoader.RemoveResourceFormatLoader(_modelLoader);
            _modelLoader = null;
        }

        if (_texLoader is not null)
        {
            ResourceLoader.RemoveResourceFormatLoader(_texLoader);
            _texLoader = null;
        }

        if (_skyLoader is not null)
        {
            ResourceLoader.RemoveResourceFormatLoader(_skyLoader);
            _skyLoader = null;
        }

        if (_areaLoader is not null)
        {
            ResourceLoader.RemoveResourceFormatLoader(_areaLoader);
            _areaLoader = null;
        }

        if (_mapLoader is not null)
        {
            ResourceLoader.RemoveResourceFormatLoader(_mapLoader);
            _mapLoader = null;
        }

        M3SceneLoader.SetResolver(null);
        TexResourceLoader.SetResolver(null);
        SkySceneLoader.SetResolver(null);
        WildStar.Sky.SkySceneBuilder.SetResolver(null);
        AreaSceneLoader.SetResolver(null);
        WildStar.Area.AreaTables.SetResolver(null);
        WildStar.Area.AreaTables.SetFileSystem(null);
        WildStar.Area.TerrainSplat.ClearCache();
        WildStar.Model.M3TextureCache.SetResolver(null);
        WildStar.Model.M3TextureCache.SetGroupResolver(null);
        WildStar.Model.M3SceneBuilder.SetFileSystem(null);
    }

    private const int MountWaitSeconds = 120;

    private WsFileSystem? MountedFileSystem()
    {
        WsFileSystem? filesystem = _filesystem;
        if (filesystem is not null)
        {
            return filesystem;
        }

        Task? mount = _mountTask;
        if (mount is null || mount.IsCompleted)
        {
            return _filesystem;
        }

        try
        {
            mount.Wait(TimeSpan.FromSeconds(MountWaitSeconds));
        }
        catch (Exception)
        {
        }

        return _filesystem;
    }

    private byte[]? ReadModelBytes(string path)
    {
        WsFileSystem? filesystem = MountedFileSystem();
        if (filesystem is null)
        {
            GD.PushError("[wildstar_mount] " + path + ": no archives are mounted (the mount " +
                         "failed or was remounting) — check the earlier [wildstar_mount] lines " +
                         "and the " + GameDirectorySetting + " setting, then use \"Remount archives\"");
            return null;
        }

        if (!filesystem.TryGetFile(path, out WsFile file))
        {
            bool found = false;
            foreach (WsArchive archive in filesystem.Archives)
            {
                if (filesystem.TryGetFile(archive.Name + "://" + path, out file))
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                GD.PushError("[wildstar_mount] " + path + ": not in any of the " +
                             filesystem.Archives.Count + " mounted archive(s)");
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

    private const int ThumbnailSize = 32;
    private const ulong MaxThumbnailBytes = 4 * 1024 * 1024;

    private readonly System.Collections.Generic.Dictionary<string, ImageTexture?> _thumbnails =
        new(StringComparer.OrdinalIgnoreCase);

    private void ApplyFileIcon(TreeItem item, WsFile file, Texture2D fallback)
    {
        if (IsTexture(file.Name))
        {
            ImageTexture? thumb = Thumbnail(file);
            if (thumb is not null)
            {
                item.SetIcon(0, thumb);
                item.SetIconModulate(0, Colors.White);
                item.SetIconMaxWidth(0, ThumbnailSize);
                return;
            }
        }

        item.SetIcon(0, IconFor(file.Name) ?? fallback);
    }

    private ImageTexture? Thumbnail(WsFile file)
    {
        string key = file.QualifiedPath;
        if (_thumbnails.TryGetValue(key, out ImageTexture? cached))
        {
            return cached;
        }

        ImageTexture? thumb = BuildThumbnail(file);
        _thumbnails[key] = thumb;
        return thumb;
    }

    private static ImageTexture? BuildThumbnail(WsFile file)
    {
        if (file.UncompressedSize == 0 || file.UncompressedSize > MaxThumbnailBytes)
        {
            return null;
        }

        try
        {
            byte[] head = file.ReadPrefix(WildStar.Texture.TexFile.DataStart);
            if (!WildStar.Texture.TexFile.TryThumbnailExtent(head, ThumbnailSize, out int needed, out _))
            {
                return null;
            }

            byte[] bytes = needed <= head.Length ? head : file.ReadPrefix(needed);
            if (!WildStar.Texture.TexFile.TryDecodeThumbnail(
                    bytes, ThumbnailSize, out int w, out int h, out byte[] rgba, out _))
            {
                return null;
            }

            Image image = Image.CreateFromData(w, h, false, Image.Format.Rgba8, rgba);

            if (Math.Max(w, h) > ThumbnailSize)
            {
                float scale = (float)ThumbnailSize / Math.Max(w, h);
                image.Resize(
                    Math.Max(1, (int)(w * scale)),
                    Math.Max(1, (int)(h * scale)),
                    Image.Interpolation.Bilinear);
            }

            return ImageTexture.CreateFromImage(image);
        }
        catch (Exception)
        {
            return null;
        }
    }

    private string[] FindTextureVariants(string basePath)
    {
        if (_filesystem is null)
        {
            return Array.Empty<string>();
        }

        int slash = basePath.LastIndexOf('/');
        if (slash <= 0)
        {
            return Array.Empty<string>();
        }

        string directory = basePath[..slash];
        string leaf = basePath[(slash + 1)..];
        var found = new System.Collections.Generic.List<string>();

        foreach (WsArchive archive in _filesystem.Archives)
        {
            if (!_filesystem.TryGetDirectory(archive.Name + "://" + directory,
                                             out WsDirectory folder))
            {
                continue;
            }

            foreach (WsFile candidate in folder.Files)
            {
                if (candidate.Name.Length > leaf.Length + 4 &&
                    candidate.Name.StartsWith(leaf + ".", StringComparison.OrdinalIgnoreCase) &&
                    candidate.Name.EndsWith(".tex", StringComparison.OrdinalIgnoreCase))
                {
                    found.Add(directory + "/" + candidate.Name);
                }
            }

            if (found.Count > 0)
            {
                break;
            }
        }

        found.Sort(StringComparer.OrdinalIgnoreCase);
        return found.ToArray();
    }

    private void OpenTexture(WsFile file)
    {
        byte[] bytes;
        try
        {
            bytes = file.ReadAllBytes();
        }
        catch (Exception exception)
        {
            GD.PushError("[wildstar_mount] " + file.QualifiedPath + ": " + exception.Message);
            return;
        }

        ImageTexture? texture = TexResourceLoader.Decode(bytes, out string error);
        if (texture is null)
        {
            GD.PushWarning("[wildstar_mount] " + file.QualifiedPath + ": " + error);
            return;
        }

        texture.ResourceName = TexResourceLoader.NameOf(file.QualifiedPath);

        GD.Print("[wildstar_mount] " + file.QualifiedPath + " — " +
            texture.GetWidth() + "x" + texture.GetHeight());

        EditorInterface.Singleton.InspectObject(texture);
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
