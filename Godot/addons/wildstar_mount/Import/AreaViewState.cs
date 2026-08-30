#if TOOLS
using System;
using System.Collections.Generic;
using System.Security.Cryptography;
using System.Text;
using Godot;
using WildStar.Area;

namespace WildStar.Editor;

internal static class AreaViewState
{
    private const float Pitch = 0.55f;
    private const float Yaw = -0.75f;
    private const float DistanceFactor = 1.3f;
    private const float FarPlane = 40000.0f;

    private static readonly object Gate = new();

    private static readonly Dictionary<string, string?> Pending = new(StringComparer.Ordinal);

    public static void BeginOpen(string scenePath)
    {
        string? existing = null;
        try
        {
            string file = StateFile(scenePath);
            if (Godot.FileAccess.FileExists(file))
            {
                existing = Godot.FileAccess.GetFileAsString(file);
            }
        }
        catch (Exception)
        {
            existing = null;
        }

        lock (Gate)
        {
            Pending[scenePath] = existing;
        }
    }

    public static void EndOpen(string scenePath)
    {
        lock (Gate)
        {
            Pending.Remove(scenePath);
        }
    }

    public static void Prime(string scenePath, AreaRoot root, AreaTileCoord tile)
    {
        if (OS.GetThreadCallerId() != OS.GetMainThreadId())
        {
            return;
        }

        bool pending;
        string? snapshot;
        lock (Gate)
        {
            pending = Pending.Remove(scenePath, out snapshot);
        }

        try
        {
            string file = StateFile(scenePath);

            if (pending && snapshot is not null)
            {
                WriteText(file, snapshot);
                return;
            }

            if (!pending && Godot.FileAccess.FileExists(file))
            {
                return;
            }

            Seed(file, Bounds(root, tile));
        }
        catch (Exception exception)
        {
            GD.PushWarning("[wildstar_mount] could not set up the 3D view for " + scenePath + ": " +
                           exception.Message);
        }
    }

    public static void Prime(string scenePath, MapRoot root)
    {
        if (OS.GetThreadCallerId() != OS.GetMainThreadId())
        {
            return;
        }

        bool pending;
        string? snapshot;
        lock (Gate)
        {
            pending = Pending.Remove(scenePath, out snapshot);
        }

        try
        {
            string file = StateFile(scenePath);

            if (pending && snapshot is not null)
            {
                WriteText(file, snapshot);
                return;
            }

            if (!pending && Godot.FileAccess.FileExists(file))
            {
                return;
            }

            Seed(file, MapBounds(root));
        }
        catch (Exception exception)
        {
            GD.PushWarning("[wildstar_mount] could not set up the 3D view for " + scenePath + ": " +
                           exception.Message);
        }
    }

    private static Aabb MapBounds(MapRoot root)
    {
        var bounds = new Aabb();
        bool any = false;
        for (int pass = 0; pass < 2 && !any; pass++)
        {
            foreach (Node child in root.GetChildren())
            {
                if (child is not AreaRoot tile || (pass == 0 && tile.LowDetail))
                {
                    continue;
                }

                Aabb tileBounds = Bounds(tile, new AreaTileCoord(tile.TileX, tile.TileZ, tile.LowDetail));
                bounds = any ? bounds.Merge(tileBounds) : tileBounds;
                any = true;
            }
        }

        if (any)
        {
            return bounds;
        }

        var focus = new AreaTileCoord(root.FocusX, root.FocusZ, false);
        return new Aabb(new Vector3(focus.OriginX, 0.0f, focus.OriginZ),
                        new Vector3(AreaTerrain.TileSize, 0.0f, AreaTerrain.TileSize));
    }

    private static void Seed(string file, Aabb bounds)
    {
        float extent = Math.Max(bounds.Size.X, bounds.Size.Z);

        var viewport = new Godot.Collections.Dictionary
        {
            ["position"] = bounds.GetCenter(),
            ["x_rotation"] = Pitch,
            ["y_rotation"] = Yaw,
            ["distance"] = extent * DistanceFactor,
        };

        var view3d = new Godot.Collections.Dictionary
        {
            ["viewports"] = new Godot.Collections.Array { viewport },
            ["zfar"] = FarPlane,
        };

        var config = new ConfigFile();
        config.SetValue("editor_states", "3D", view3d);
        config.SetValue("editor_states", "$selected_nodes",
            new Godot.Collections.Array { new NodePath(".") });

        Error saved = config.Save(file);
        if (saved != Error.Ok)
        {
            throw new InvalidOperationException("saving " + file + " failed: " + saved);
        }
    }

    private static void WriteText(string file, string text)
    {
        using Godot.FileAccess access = Godot.FileAccess.Open(file, Godot.FileAccess.ModeFlags.Write)
            ?? throw new InvalidOperationException("cannot write " + file + ": " + Godot.FileAccess.GetOpenError());
        access.StoreString(text);
    }

    private static string StateFile(string scenePath)
    {
        string directory = EditorInterface.Singleton.GetEditorPaths().GetProjectSettingsDir();
        string digest = Convert.ToHexString(MD5.HashData(Encoding.UTF8.GetBytes(scenePath)))
            .ToLowerInvariant();
        return directory.PathJoin(scenePath.GetFile() + "-editstate-" + digest + ".cfg");
    }

    private static Aabb Bounds(AreaRoot root, AreaTileCoord tile)
    {
        var bounds = new Aabb();
        bool any = false;

        if (root.GetNodeOrNull<Node3D>(AreaRoot.ChunksNode) is Node3D chunks)
        {
            foreach (Node child in chunks.GetChildren())
            {
                if (child is not MeshInstance3D instance || instance.Mesh is null)
                {
                    continue;
                }

                Aabb local = instance.Mesh.GetAabb();
                var world = new Aabb(root.Position + instance.Position + local.Position, local.Size);
                bounds = any ? bounds.Merge(world) : world;
                any = true;
            }
        }

        if (!any)
        {
            bounds = new Aabb(root.Position, new Vector3(tile.TileSize, 0.0f, tile.TileSize));
        }

        return bounds;
    }
}
#endif
