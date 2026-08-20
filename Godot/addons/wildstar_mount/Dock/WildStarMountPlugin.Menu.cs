#if TOOLS
using System;
using Godot;
using WildStar.Archive;
using WildStar.Audio.Bank;

namespace WildStar.Editor;

public partial class WildStarMountPlugin
{
    private const int MenuExtract = 0;
    private const int MenuExtractFolder = 1;
    private const int MenuCopyPath = 2;
    private const int MenuInfo = 3;
    private const int MenuRemount = 4;
    private const int MenuPlay = 5;
    private const int MenuStop = 6;
    private const int MenuPlace = 7;
    private const int MenuOpenTexture = 8;
    private const int MenuOpenTable = 9;

    private void InstallBridge(Tree tree)
    {
        var script = GD.Load<GDScript>("res://addons/wildstar_mount/ws_signal_bridge.gd");
        if (script is null)
        {
            GD.PushWarning("[wildstar_mount] signal bridge script missing");
            return;
        }

        _bridge = (GodotObject)script.New();
        _bridge.Call(
            "install",
            tree,
            Callable.From<TreeItem, bool>(IsOurs),
            Callable.From<TreeItem>(OnActivated),
            Callable.From<TreeItem, Vector2, long>(OnMouseSelected),
            Callable.From<TreeItem>(OnSelected));
    }

    private bool IsOurs(TreeItem item)
    {
        if (item is null || !GodotObject.IsInstanceValid(item))
        {
            return false;
        }

        string meta = item.GetMetadata(0).AsString();
        return meta == PlaceholderMeta ||
               meta.StartsWith(RootPrefix, StringComparison.Ordinal) ||
               meta.StartsWith(DirectoryPrefix, StringComparison.Ordinal) ||
               meta.StartsWith(BankPrefix, StringComparison.Ordinal) ||
               meta.StartsWith(BankSoundPrefix, StringComparison.Ordinal) ||
               IsFileMeta(meta);
    }

    private void OnActivated(TreeItem item)
    {
        string meta = item.GetMetadata(0).AsString();

        if (meta.StartsWith(BankSoundPrefix, StringComparison.Ordinal))
        {
            if (TryGetBankSoundBytes(meta[BankSoundPrefix.Length..], out byte[] audio))
            {
                PlayPreview(meta[BankSoundPrefix.Length..], audio);
            }

            return;
        }

        if (!TryGetFileMeta(meta, out WsFile file))
        {
            EnsurePopulated(item);
            item.Collapsed = !item.Collapsed;
            return;
        }

        if (IsAudio(file.Name))
        {
            PlayPreview(file);
            return;
        }

        if (IsModel(file.Name))
        {
            PlaceModel(file);
            return;
        }

        if (IsTexture(file.Name))
        {
            OpenTexture(file);
            return;
        }

        if (IsTable(file.Name))
        {
            OpenTable(file);
            return;
        }

        ExtractFile(file);
    }

    private void OnMouseSelected(TreeItem item, Vector2 position, long button)
    {
        if (button != (long)MouseButton.Right)
        {
            return;
        }

        string meta = item.GetMetadata(0).AsString();
        bool isFile = IsFileMeta(meta);
        bool isRoot = meta.StartsWith(RootPrefix, StringComparison.Ordinal);
        bool isBankSound = meta.StartsWith(BankSoundPrefix, StringComparison.Ordinal);

        PopupMenu menu = EnsureMenu();
        menu.Clear();

        if (isFile || isBankSound)
        {
            if (isBankSound || IsAudio(item.GetText(0)))
            {
                menu.AddItem("Play", MenuPlay);
                menu.AddItem("Stop", MenuStop);
                menu.AddSeparator();
            }
            else if (IsModel(item.GetText(0)))
            {
                menu.AddItem("Add to scene", MenuPlace);
                menu.AddSeparator();
            }
            else if (IsTexture(item.GetText(0)))
            {
                menu.AddItem("Open texture", MenuOpenTexture);
                menu.AddSeparator();
            }
            else if (IsTable(item.GetText(0)))
            {
                menu.AddItem("Open table", MenuOpenTable);
                menu.AddSeparator();
            }

            menu.AddItem("Extract", MenuExtract);
        }
        else
        {
            menu.AddItem(isRoot ? "Extract whole archive…" : "Extract folder…", MenuExtractFolder);
        }

        menu.AddItem("Copy path", MenuCopyPath);
        menu.AddItem("Print info", MenuInfo);
        menu.AddSeparator();
        menu.AddItem("Remount archives", MenuRemount);

        menu.ResetSize();
        menu.Position = DisplayServer.MouseGetPosition();
        menu.Popup();
    }

    private void OnMenuPressed(long id)
    {
        Tree? tree = ResolveTree();
        TreeItem? item = tree?.GetSelected();
        if (item is null || !GodotObject.IsInstanceValid(item))
        {
            if (id == MenuRemount)
            {
                Remount();
            }

            return;
        }

        string meta = item.GetMetadata(0).AsString();

        switch (id)
        {
            case MenuRemount:
                Remount();
                return;

            case MenuCopyPath:
                DisplayServer.ClipboardSet(StripPrefix(meta));
                GD.Print("[wildstar_mount] copied " + StripPrefix(meta));
                return;

            case MenuStop:
                StopPreview();
                return;
        }

        if (_filesystem is null)
        {
            return;
        }

        if (meta.StartsWith(BankSoundPrefix, StringComparison.Ordinal))
        {
            if (!TryGetBankSound(meta[BankSoundPrefix.Length..], out WwiseBankSound sound))
            {
                return;
            }

            if (id == MenuPlay)
            {
                if (TryGetBankSoundBytes(meta[BankSoundPrefix.Length..], out byte[] audio))
                {
                    PlayPreview(meta[BankSoundPrefix.Length..], audio);
                }
            }
            else if (id == MenuExtract)
            {
                ExtractBankSound(meta[BankSoundPrefix.Length..], sound);
            }
            else if (id == MenuInfo)
            {
                GD.Print("[wildstar_mount] " + meta[BankSoundPrefix.Length..] + " — " +
                    FormatSize((ulong)sound.Length) + " embedded");
            }

            return;
        }

        if (TryGetFileMeta(meta, out WsFile file))
        {
            if (id == MenuPlay)
            {
                PlayPreview(file);
            }
            else if (id == MenuPlace)
            {
                PlaceModel(file);
            }
            else if (id == MenuOpenTexture)
            {
                OpenTexture(file);
            }
            else if (id == MenuOpenTable)
            {
                OpenTable(file);
            }
            else if (id == MenuExtract)
            {
                ExtractFile(file);
            }
            else if (id == MenuInfo)
            {
                GD.Print("[wildstar_mount] " + Describe(file).Replace("\n", "\n    "));
            }

            return;
        }

        WsDirectory? directory = ResolveDirectory(meta);
        if (directory is null)
        {
            return;
        }

        if (id == MenuExtractFolder)
        {
            ConfirmExtractDirectory(directory);
        }
        else if (id == MenuInfo)
        {
            GD.Print("[wildstar_mount] " + directory.QualifiedPath + "\n    " +
                directory.Directories.Count + " directories, " + directory.Files.Count + " files");
        }
    }

    private bool IsFileMeta(string meta) => TryGetFileMeta(meta, out _);

    private bool TryGetFileMeta(string meta, out WsFile file)
    {
        file = null!;
        return _filesystem is not null && _filesystem.TryGetFile(meta, out file);
    }

    private WsDirectory? ResolveDirectory(string meta)
    {
        if (_filesystem is null)
        {
            return null;
        }

        if (meta.StartsWith(RootPrefix, StringComparison.Ordinal))
        {
            return _filesystem.TryGetArchive(meta[RootPrefix.Length..], out WsArchive archive)
                ? archive.Root
                : null;
        }

        if (meta.StartsWith(DirectoryPrefix, StringComparison.Ordinal) &&
            _filesystem.TryGetDirectory(meta[DirectoryPrefix.Length..], out WsDirectory directory))
        {
            return directory;
        }

        return null;
    }

    private PopupMenu EnsureMenu()
    {
        if (_menu is not null && GodotObject.IsInstanceValid(_menu))
        {
            return _menu;
        }

        _menu = new PopupMenu();
        _menu.IdPressed += OnMenuPressed;
        EditorInterface.Singleton.GetBaseControl().AddChild(_menu);
        return _menu;
    }

    private static string StripPrefix(string meta)
    {
        if (meta.StartsWith(DirectoryPrefix, StringComparison.Ordinal))
        {
            return meta[DirectoryPrefix.Length..];
        }

        if (meta.StartsWith(BankPrefix, StringComparison.Ordinal))
        {
            return meta[BankPrefix.Length..];
        }

        if (meta.StartsWith(BankSoundPrefix, StringComparison.Ordinal))
        {
            return meta[BankSoundPrefix.Length..];
        }

        if (meta.StartsWith(RootPrefix, StringComparison.Ordinal))
        {
            return meta[RootPrefix.Length..] + "://";
        }

        return meta;
    }
}
#endif
