#if TOOLS
using System.Collections.Generic;
using System.IO;
using Godot;
using WildStar.Archive;

namespace WildStar.Editor;

public partial class WildStarMountPlugin
{
    private const string PickerHint =
        "\n\nClick to pick Wildstar.exe. Everything under its folder is searched for .index " +
        "files, and each one that has a matching .archive is mounted.";

    private void EnsurePicker()
    {
        if (_picker is not null && GodotObject.IsInstanceValid(_picker))
        {
            UpdatePicker();
            return;
        }

        if (FindByClass(GetTree().Root, "FileSystemDock", 0) is not Node dock)
        {
            return;
        }

        Node host = dock;
        foreach (Node child in dock.GetChildren())
        {
            if (child is VBoxContainer box)
            {
                host = box;
                break;
            }
        }

        _picker = new Button
        {
            Alignment = HorizontalAlignment.Left,
            ClipText = true,
            Icon = FindIcon("Load", "Folder", "File"),
        };

        _picker.Pressed += OnPickerPressed;
        host.AddChild(_picker);
        host.MoveChild(_picker, 0);
        UpdatePicker();
    }

    private void KeepPickerOnTop()
    {
        if (_picker is null || !GodotObject.IsInstanceValid(_picker))
        {
            EnsurePicker();
            return;
        }

        if (_picker.GetParent() is Node host && _picker.GetIndex() != 0)
        {
            host.MoveChild(_picker, 0);
        }
    }

    private void UpdatePicker()
    {
        if (_picker is null || !GodotObject.IsInstanceValid(_picker))
        {
            return;
        }

        if (_mounting)
        {
            _picker.Text = "WildStar: mounting…";
            _picker.TooltipText = "Reading archive headers." + PickerHint;
            return;
        }

        if (_filesystem is null || _filesystem.Archives.Count == 0)
        {
            _picker.Text = "Select Wildstar.exe…";
            _picker.TooltipText = _filesystem is null
                ? "No WildStar install found." + PickerHint
                : "No archives under " + _filesystem.GameDirectory + PickerHint;
            return;
        }

        var names = new List<string>(_filesystem.Archives.Count);
        foreach (WsArchive archive in _filesystem.Archives)
        {
            names.Add(archive.Name);
        }

        _picker.Text = "WildStar: " + _filesystem.Archives.Count + " archive(s)";
        _picker.TooltipText =
            _filesystem.GameDirectory + "\n" + string.Join(", ", names) + PickerHint;
    }

    private void OnPickerPressed()
    {
        if (_picking is null || !GodotObject.IsInstanceValid(_picking))
        {
            _picking = new EditorFileDialog
            {
                Title = "Select Wildstar.exe",
                FileMode = EditorFileDialog.FileModeEnum.OpenFile,
                Access = EditorFileDialog.AccessEnum.Filesystem,
            };

            _picking.ClearFilters();
            _picking.AddFilter("Wildstar.exe", "WildStar client");
            _picking.AddFilter("*.exe", "Executable");
            _picking.FileSelected += OnExeSelected;
            EditorInterface.Singleton.GetBaseControl().AddChild(_picking);
        }

        string current = _filesystem?.GameDirectory ?? string.Empty;
        if (current.Length != 0 && Directory.Exists(current))
        {
            _picking.CurrentDir = current;
        }

        _picking.PopupCentered(new Vector2I(900, 640));
    }

    private void OnExeSelected(string path)
    {
        string? directory = Path.GetDirectoryName(ProjectSettings.GlobalizePath(path));
        if (directory is null || directory.Length == 0)
        {
            return;
        }

        ProjectSettings.SetSetting(GameDirectorySetting, directory);
        Error saved = ProjectSettings.Save();
        if (saved != Error.Ok)
        {
            GD.PushWarning("[wildstar_mount] could not save " + GameDirectorySetting +
                " (" + saved + "); the choice holds for this session only");
        }

        GD.Print("[wildstar_mount] scanning " + directory);
        Remount();
        UpdatePicker();
    }
}
#endif
