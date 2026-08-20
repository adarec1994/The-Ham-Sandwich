#if TOOLS
using System;
using Godot;
using WildStar.Archive;

namespace WildStar.Editor;

public partial class WildStarMountPlugin
{
    private M3SceneLoader? _modelLoader;
    private TexResourceLoader? _texLoader;

    private static bool IsModel(string name) =>
        name.EndsWith(".m3", StringComparison.OrdinalIgnoreCase);

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

    private void InstallModelLoader()
    {
        M3SceneLoader.SetResolver(ReadModelBytes);
        TexResourceLoader.SetResolver(ReadModelBytes);
        WildStar.Model.M3TextureCache.SetResolver(ReadModelBytes);
        WildStar.Model.M3SceneBuilder.SetFileSystem(() => _filesystem);
        _modelLoader = new M3SceneLoader();
        _texLoader = new TexResourceLoader();
        ResourceLoader.AddResourceFormatLoader(_modelLoader);
        ResourceLoader.AddResourceFormatLoader(_texLoader);
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

        M3SceneLoader.SetResolver(null);
        TexResourceLoader.SetResolver(null);
        WildStar.Model.M3TextureCache.SetResolver(null);
        WildStar.Model.M3SceneBuilder.SetFileSystem(null);
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
            if (!WildStar.Texture.TexFile.TryDecodeThumbnail(
                    file.ReadAllBytes(), ThumbnailSize, out int w, out int h,
                    out byte[] rgba, out _))
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
