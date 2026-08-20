#if TOOLS
using System;
using System.Collections.Generic;
using Godot;
using WildStar.Archive;

namespace WildStar.Editor;

public partial class WildStarMountPlugin
{
    private void Inject()
    {
        if (_filesystem is null)
        {
            return;
        }

        Tree? tree = ResolveTree();
        TreeItem? root = tree?.GetRoot();
        if (tree is null || root is null)
        {
            return;
        }

        if (_hookedTree is null || !GodotObject.IsInstanceValid(_hookedTree) || _hookedTree != tree)
        {
            Unhook();
            _hookedTree = tree;
            tree.ItemCollapsed += OnItemCollapsed;
            InstallBridge(tree);
        }

        RemoveRoots();

        if (_filesystem.Archives.Count == 0)
        {
            GD.Print("[wildstar_mount] no archives under " + _filesystem.GameDirectory);
            foreach (string warning in _filesystem.Warnings)
            {
                GD.Print("[wildstar_mount]   " + warning);
            }

            return;
        }

        Texture2D folder = GetFolderIcon();

        foreach (WsArchive archive in _filesystem.Archives)
        {
            TreeItem item = tree.CreateItem(root);
            item.SetText(0, archive.Name + "://");
            item.SetMetadata(0, RootPrefix + archive.Name);
            item.SetSelectable(0, false);
            item.SetIcon(0, folder);
            item.SetIconModulate(0, ArchiveColor);
            item.SetTooltipText(0, archive.IndexPath + "\n" + archive.DataPath +
                "\n" + archive.BlockCount.ToString("N0") + " stored blocks");
            AddPlaceholder(tree, item);
            item.Collapsed = true;
            _roots.Add(item);
        }

        TreeItem? resRoot = FindResRoot(root);
        if (resRoot is not null)
        {
            foreach (TreeItem item in _roots)
            {
                item.MoveBefore(resRoot);
            }
        }

        RestoreExpansion();
    }

    private void Unhook()
    {
        if (_bridge is not null && GodotObject.IsInstanceValid(_bridge))
        {
            _bridge.Call("uninstall");
        }

        _bridge = null;

        if (_hookedTree is not null && GodotObject.IsInstanceValid(_hookedTree))
        {
            _hookedTree.ItemCollapsed -= OnItemCollapsed;
        }

        _hookedTree = null;
    }

    private void RemoveRoots()
    {
        foreach (TreeItem item in _roots)
        {
            if (!GodotObject.IsInstanceValid(item))
            {
                continue;
            }

            item.GetParent()?.RemoveChild(item);
            item.Free();
        }

        _roots.Clear();
        _pending.Clear();
    }

    private void RestoreExpansion()
    {
        if (_expanded.Count == 0)
        {
            return;
        }

        foreach (TreeItem item in _roots)
        {
            RestoreExpansion(item);
        }
    }

    private void RestoreExpansion(TreeItem item)
    {
        if (!_expanded.Contains(item.GetMetadata(0).AsString()))
        {
            return;
        }

        EnsurePopulated(item);
        item.Collapsed = false;

        for (TreeItem? child = item.GetFirstChild(); child is not null; child = child.GetNext())
        {
            if (child.GetMetadata(0).AsString().StartsWith(DirectoryPrefix, StringComparison.Ordinal))
            {
                RestoreExpansion(child);
            }
        }
    }

    private void OnItemCollapsed(TreeItem item)
    {
        if (!IsOurs(item))
        {
            return;
        }

        string meta = item.GetMetadata(0).AsString();
        if (item.Collapsed)
        {
            _expanded.Remove(meta);
        }
        else
        {
            _expanded.Add(meta);
        }

        if (item.Collapsed || _filesystem is null || !HasPlaceholder(item))
        {
            return;
        }

        _pending.Add(item);
        CallDeferred(nameof(FlushPending));
    }

    private void FlushPending()
    {
        if (_pending.Count == 0)
        {
            return;
        }

        var work = new List<TreeItem>(_pending);
        _pending.Clear();

        foreach (TreeItem item in work)
        {
            if (GodotObject.IsInstanceValid(item))
            {
                EnsurePopulated(item);
            }
        }
    }

    private void EnsurePopulated(TreeItem item)
    {
        if (!HasPlaceholder(item))
        {
            return;
        }

        TreeItem placeholder = item.GetChild(0);
        item.RemoveChild(placeholder);
        placeholder.Free();

        string meta = item.GetMetadata(0).AsString();

        if (meta.StartsWith(RootPrefix, StringComparison.Ordinal))
        {
            if (_filesystem is not null &&
                _filesystem.TryGetArchive(meta[RootPrefix.Length..], out WsArchive archive))
            {
                Populate(item, archive.Root);
            }

            return;
        }

        if (meta.StartsWith(BankPrefix, StringComparison.Ordinal))
        {
            PopulateBank(item, meta[BankPrefix.Length..]);
            return;
        }

        if (meta.StartsWith(DirectoryPrefix, StringComparison.Ordinal) &&
            _filesystem is not null &&
            _filesystem.TryGetDirectory(meta[DirectoryPrefix.Length..], out WsDirectory directory))
        {
            Populate(item, directory);
        }
    }

    private void Populate(TreeItem parent, WsDirectory directory)
    {
        Tree? tree = ResolveTree();
        if (tree is null)
        {
            return;
        }

        Texture2D folder = GetFolderIcon();
        Texture2D file = GetFileIcon();

        foreach (WsDirectory child in directory.Directories)
        {
            TreeItem item = tree.CreateItem(parent);
            item.SetText(0, child.Name);
            item.SetMetadata(0, DirectoryPrefix + child.QualifiedPath);
            item.SetIcon(0, folder);
            item.SetIconModulate(0, ArchiveColor);
            item.SetTooltipText(0, child.QualifiedPath);
            AddPlaceholder(tree, item);
            item.Collapsed = true;
        }

        foreach (WsFile child in directory.Files)
        {
            TreeItem item = tree.CreateItem(parent);
            item.SetText(0, child.Name);
            ApplyFileIcon(item, child, file);
            item.SetTooltipText(0, Describe(child));

            if (IsBank(child.Name))
            {
                item.SetMetadata(0, BankPrefix + child.QualifiedPath);
                AddPlaceholder(tree, item);
                item.Collapsed = true;
                continue;
            }

            item.SetMetadata(0, child.QualifiedPath);
        }
    }

    private static bool HasPlaceholder(TreeItem item) =>
        item.GetChildCount() == 1 &&
        item.GetChild(0).GetMetadata(0).AsString() == PlaceholderMeta;

    private static void AddPlaceholder(Tree tree, TreeItem parent)
    {
        TreeItem placeholder = tree.CreateItem(parent);
        placeholder.SetText(0, "…");
        placeholder.SetSelectable(0, false);
        placeholder.SetMetadata(0, PlaceholderMeta);
    }

    private static TreeItem? FindResRoot(TreeItem root)
    {
        for (TreeItem? child = root.GetFirstChild(); child is not null; child = child.GetNext())
        {
            if (child.GetText(0).StartsWith("res://", StringComparison.Ordinal))
            {
                return child;
            }
        }

        return null;
    }
}
#endif
