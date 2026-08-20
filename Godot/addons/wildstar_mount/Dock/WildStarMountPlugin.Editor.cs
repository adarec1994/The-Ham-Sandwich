#if TOOLS
using System;
using System.Collections.Generic;
using System.IO;
using Godot;
using WildStar.Archive;

namespace WildStar.Editor;

public partial class WildStarMountPlugin
{
    private static readonly Dictionary<string, string[]> IconsByExtension =
        new(StringComparer.OrdinalIgnoreCase)
        {
            [".tex"] = new[] { "ImageTexture", "Texture2D", "Image" },
            [".m3"] = new[] { "MeshInstance3D", "ArrayMesh", "Mesh" },
            [".i3"] = new[] { "MultiMeshInstance3D", "MeshInstance3D", "Mesh" },
            [".tbl"] = new[] { "PackedDataContainer", "ItemList", "FileList" },
            [".area"] = new[] { "PackedScene", "Node3D" },
            [".sky"] = new[] { "WorldEnvironment", "Environment" },
            [".lua"] = new[] { "Script", "GDScript", "TextFile" },
            [".xml"] = new[] { "TextFile", "FileList" },
            [".txt"] = new[] { "TextFile", "File" },
            [".bnk"] = new[] { "AudioStreamPlayer", "AudioStream" },
            [".wem"] = new[] { "AudioStreamWAV", "AudioStream" },
            [".ogg"] = new[] { "AudioStreamOggVorbis", "AudioStream" },
            [".anm"] = new[] { "Animation", "AnimationPlayer" },
            [".ttf"] = new[] { "FontFile", "Font" },
            [".otf"] = new[] { "FontFile", "Font" },
            [".bin"] = new[] { "FileBroken", "File" },
        };

    private Tree? ResolveTree()
    {
        if (GodotObject.IsInstanceValid(_tree))
        {
            return _tree;
        }

        Node? dock = FindByClass(GetTree().Root, "FileSystemDock", 0);
        _tree = dock is null ? null : FindByClass(dock, "Tree", 0) as Tree;
        return _tree;
    }

    private static Node? FindByClass(Node node, string className, int depth)
    {
        if (depth > 12)
        {
            return null;
        }

        if (node.GetClass() == className)
        {
            return node;
        }

        foreach (Node child in node.GetChildren())
        {
            Node? found = FindByClass(child, className, depth + 1);
            if (found is not null)
            {
                return found;
            }
        }

        return null;
    }

    private static void FreeControl<T>(ref T? control) where T : Node
    {
        if (control is not null && GodotObject.IsInstanceValid(control))
        {
            control.GetParent()?.RemoveChild(control);
            control.QueueFree();
        }

        control = null;
    }

    private static void EnsureSetting(string name, Variant fallback)
    {
        if (!ProjectSettings.HasSetting(name))
        {
            ProjectSettings.SetSetting(name, fallback);
        }

        ProjectSettings.SetInitialValue(name, fallback);
    }

    private static Texture2D GetFolderIcon() =>
        EditorInterface.Singleton.GetEditorTheme().GetIcon("Folder", "EditorIcons");

    private static Texture2D GetFileIcon() =>
        EditorInterface.Singleton.GetEditorTheme().GetIcon("File", "EditorIcons");

    private static Texture2D? IconFor(string fileName) =>
        IconsByExtension.TryGetValue(Path.GetExtension(fileName), out string[]? candidates)
            ? FindIcon(candidates)
            : null;

    private static Texture2D? FindIcon(params string[] candidates)
    {
        Theme theme = EditorInterface.Singleton.GetEditorTheme();
        foreach (string name in candidates)
        {
            if (theme.HasIcon(name, "EditorIcons"))
            {
                return theme.GetIcon(name, "EditorIcons");
            }
        }

        return null;
    }

    private static string Describe(WsFile file)
    {
        string line = file.QualifiedPath + "\n" +
            FormatSize(file.UncompressedSize) + "  ·  " + file.Compression +
            " (" + FormatSize(file.CompressedSize) + " stored)";

        if (file.WriteTime != 0)
        {
            line += "\n" + file.WriteTimeUtc.ToString("yyyy-MM-dd HH:mm:ss") + " UTC";
        }

        return line;
    }

    private static string FormatSize(ulong bytes)
    {
        string[] units = { "B", "KiB", "MiB", "GiB", "TiB" };
        double value = bytes;
        int unit = 0;

        while (value >= 1024 && unit < units.Length - 1)
        {
            value /= 1024;
            unit++;
        }

        return unit == 0 ? bytes + " B" : value.ToString("0.##") + " " + units[unit];
    }
}
#endif
