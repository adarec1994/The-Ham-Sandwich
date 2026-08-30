using System;
using System.Collections.Generic;
using Godot;
using WildStar.Archive;
using WildStar.Area;

namespace WildStar.Tools;

public partial class AreaSmokeTest : SceneTree
{
    private WsFileSystem? _fs;
    private int _frames;
    private string _output = "smoke.png";
    private Node3D? _map;

    public override void _Initialize()
    {
        string[] args = OS.GetCmdlineUserArgs();
        string game = args.Length > 0 ? args[0] : @"C:\Users\pwd12\OneDrive\Documents\WildStar";
        string mapName = args.Length > 1 ? args[1] : "Eastern";
        int radius = args.Length > 2 ? int.Parse(args[2]) : 1;
        _output = args.Length > 3 ? args[3] : "smoke.png";
        int focusX = args.Length > 5 ? int.Parse(args[4]) : -1;
        int focusZ = args.Length > 5 ? int.Parse(args[5]) : -1;

        if (int.TryParse(System.Environment.GetEnvironmentVariable("WS_LAYER_TEXELS"), out int texels) && texels > 0)
        {
            WildStar.Area.TerrainSplat.MaxLayerTexels = texels;
            GD.Print($"layer texture cap: {texels}");
        }

        _fs = WsFileSystem.Mount(game);
        Func<string, byte[]?> resolver = Read;
        WildStar.Model.M3TextureCache.SetResolver(resolver);
        WildStar.Model.M3SceneBuilder.SetFileSystem(() => _fs);
        WildStar.Sky.SkySceneBuilder.SetResolver(resolver);
        AreaTables.SetResolver(resolver);
        AreaTables.SetFileSystem(() => _fs);

        var world = new Node3D { Name = "World" };
        Root.AddChild(world);

        MapRoot map = MapSceneBuilder.Build(mapName, focusX, focusZ, radius, focusX < 0, m => GD.Print("  " + m));
        world.AddChild(map);
        _map = map;
        if (float.TryParse(System.Environment.GetEnvironmentVariable("WS_DETAIL_RANGE"), System.Globalization.NumberStyles.Float,
                           System.Globalization.CultureInfo.InvariantCulture, out float detailRange))
        {
            map.DetailRange = detailRange;
        }
        GD.Print($"map {mapName}: tiles={map.DetailTiles} sky={map.SkyId} focus=({map.FocusX},{map.FocusZ})");

        var focus = new AreaTileCoord(map.FocusX, map.FocusZ, false);
        float cx = focus.OriginX + AreaTerrain.TileSize * 0.5f;
        float cz = focus.OriginZ + AreaTerrain.TileSize * 0.5f;
        float cy = AverageHeight(map, cx, cz);
        var camera = new Camera3D { Far = 60000.0f, Near = 1.0f, Fov = 60.0f };
        world.AddChild(camera);
        float distance = args.Length > 6 ? float.Parse(args[6], System.Globalization.CultureInfo.InvariantCulture) : 600.0f;
        if (args.Length > 8)
        {
            cx += float.Parse(args[7], System.Globalization.CultureInfo.InvariantCulture);
            cz += float.Parse(args[8], System.Globalization.CultureInfo.InvariantCulture);
            cy = AverageHeight(map, cx, cz);
        }

        var eye = new Vector3(cx - distance, cy + distance * 0.8f, cz + distance);
        var target = new Vector3(cx, cy, cz);
        Vector3 up = Vector3.Up;
        if (distance < 0.0f)
        {
            eye = new Vector3(cx, cy - distance, cz);
            up = -Vector3.ModelFront;
        }

        camera.Transform = new Transform3D(Basis.LookingAt(target - eye, up), eye);
        camera.Current = true;
        if (map.GetNodeOrNull<Node3D>(AreaRoot.SkyNode) is Node3D skyNode)
        {
            skyNode.Position = new Vector3(skyNode.Position.X, cy, skyNode.Position.Z);
        }

        GD.Print($"camera eye={eye} target={target}");
        foreach (Node child in map.GetChildren())
        {
            if (child is not AreaRoot)
            {
                GD.Print("map child " + child.Name + " (" + child.GetType().Name + ")");
            }
        }
        if (map.GetNodeOrNull<Node3D>(AreaRoot.SkyNode) is Node3D skyRoot &&
            skyRoot.GetNodeOrNull<Node3D>("Models") is Node3D skyModels)
        {
            foreach (Node child in skyModels.GetChildren())
            {
                Aabb box = default;
                bool first = true;
                var stack = new Stack<Node>();
                stack.Push(child);
                while (stack.Count > 0)
                {
                    Node current = stack.Pop();
                    foreach (Node c in current.GetChildren())
                    {
                        stack.Push(c);
                    }

                    if (current is MeshInstance3D mi && mi.Mesh is not null)
                    {
                        Aabb local = mi.GetAabb();
                        box = first ? local : box.Merge(local);
                        first = false;
                    }
                }

                GD.Print($"sky model {child.Name} visible={(child as Node3D)?.Visible} aabb={box.Position} size={box.Size}");
            }
        }

        if (map.GetNodeOrNull<Node3D>(AreaRoot.SkyNode) is null)
        {
            var light = new DirectionalLight3D();
            world.AddChild(light);
            light.RotationDegrees = new Vector3(-50.0f, 30.0f, 0.0f);
            var env = new WorldEnvironment { Environment = new Godot.Environment { BackgroundMode = Godot.Environment.BGMode.Color, BackgroundColor = new Color(0.4f, 0.6f, 0.9f), AmbientLightSource = Godot.Environment.AmbientSource.Color, AmbientLightColor = new Color(0.4f, 0.4f, 0.45f) } };
            world.AddChild(env);
        }
    }

    private static float AverageHeight(Node map, float x, float z)
    {
        float sum = 0.0f;
        int n = 0;
        foreach (Node child in map.GetChildren())
        {
            if (child is not AreaRoot tile || tile.LowDetail)
            {
                continue;
            }

            Node3D? chunks = tile.GetNodeOrNull<Node3D>(AreaRoot.ChunksNode);
            if (chunks is null)
            {
                continue;
            }

            foreach (Node c in chunks.GetChildren())
            {
                if (c is MeshInstance3D mi && mi.Mesh is not null)
                {
                    Aabb box = mi.GetAabb();
                    sum += box.Position.Y + box.Size.Y * 0.5f;
                    n++;
                }
            }
        }

        return n > 0 ? sum / n : 0.0f;
    }

    private byte[]? Read(string path)
    {
        if (_fs is null)
        {
            return null;
        }

        if (!_fs.TryGetFile(path, out WsFile file))
        {
            bool found = false;
            foreach (WsArchive archive in _fs.Archives)
            {
                if (_fs.TryGetFile(archive.Name + "://" + path, out file))
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
        catch (Exception)
        {
            return null;
        }
    }

    public override bool _Process(double delta)
    {
        _frames++;
        if (_frames == 75)
        {
            Image image = Root.GetViewport().GetTexture().GetImage();
            Error saved = image.SavePng(_output);
            GD.Print("screenshot " + _output + " " + saved + " " + image.GetWidth() + "x" + image.GetHeight());
            _fs?.Dispose();
            return true;
        }

        return false;
    }
}
