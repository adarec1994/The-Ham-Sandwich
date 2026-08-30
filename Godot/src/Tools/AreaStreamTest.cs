using System;
using System.Collections.Generic;
using System.Linq;
using Godot;
using WildStar.Archive;
using WildStar.Area;

namespace WildStar.Tools;

public partial class AreaStreamTest : SceneTree
{
    private WsFileSystem? _fs;
    private MapRoot? _map;
    private Camera3D? _camera;
    private int _frames;
    private int _step;

    private bool _smooth;
    private readonly List<string> _log = new();

    public override void _Initialize()
    {
        string[] args = OS.GetCmdlineUserArgs();
        string game = args.Length > 0 ? args[0] : @"C:\Users\pwd12\OneDrive\Documents\WildStar";
        string mapName = args.Length > 1 ? args[1] : "Western";
        float radius = args.Length > 2 ? float.Parse(args[2], System.Globalization.CultureInfo.InvariantCulture) : 2048.0f;
        _smooth = args.Length > 3 && args[3] == "smooth";

        _fs = WsFileSystem.Mount(game);
        Func<string, byte[]?> resolver = Read;
        WildStar.Model.M3TextureCache.SetResolver(resolver);
        WildStar.Sky.SkySceneBuilder.SetResolver(resolver);
        AreaTables.SetResolver(resolver);
        AreaTables.SetFileSystem(() => _fs);

        var open = System.Diagnostics.Stopwatch.StartNew();
        MapRoot map = MapSceneBuilder.BuildStreaming(mapName, radius, m => GD.Print("  " + m));
        open.Stop();

        var world = new Node3D { Name = "World" };
        Root.AddChild(world);
        world.AddChild(map);
        _map = map;

        var focus = new AreaTileCoord(map.FocusX, map.FocusZ, false);
        _camera = new Camera3D { Far = 60000.0f, Near = 1.0f, Current = true };
        world.AddChild(_camera);
        _camera.Position = new Vector3(focus.OriginX + 256.0f, 500.0f, focus.OriginZ + 256.0f);

        GD.Print($"open: {open.ElapsedMilliseconds} ms, {map.GetChildCount()} children, " +
                 $"radius {radius}, focus ({map.FocusX},{map.FocusZ})");
    }

    private byte[]? Read(string path)
    {
        string clean = path.Replace('\\', '/');
        return _fs is not null &&
               (_fs.TryGetFile("ClientData://" + clean, out WsFile f) || _fs.TryGetFile(clean, out f))
            ? f.ReadAllBytes()
            : null;
    }

    private readonly List<double> _frameMs = new();

    private ulong _lastFrameUsec;

    private int _lastGen2, _lastGen1;

    private readonly List<string> _spikes = new();

    public override bool _Process(double delta)
    {
        _frames++;
        if (_map is null || _camera is null)
        {
            return true;
        }

        if (_frames > 2)
        {
            ulong now = Time.GetTicksUsec();
            double ms = _lastFrameUsec == 0 ? 16.7 : (now - _lastFrameUsec) / 1000.0;
            _lastFrameUsec = now;

            _frameMs.Add(ms);
            if (ms > 40.0)
            {
                int live = 0;
                foreach (Node c in _map.GetChildren(true))
                {
                    if (c is AreaRoot)
                    {
                        live++;
                    }
                }

                int g2 = GC.CollectionCount(2), g1 = GC.CollectionCount(1);
                long heap = GC.GetTotalMemory(false) / (1024 * 1024);
                _spikes.Add($"    frame {_frames,4}: {ms,8:F1} ms  ({live} tiles, {MapRoot.SkiesBuilt} skies, " +
                            $"gen2 {g2 - _lastGen2}/+{g1 - _lastGen1} gen1, heap {heap} MB)");
                _lastGen2 = g2;
                _lastGen1 = g1;
            }
        }

        if (_smooth)
        {
            _camera.Position += new Vector3(34.0f, 0.0f, 0.0f);
        }

        if (_frames % 60 != 0)
        {
            return false;
        }

        int resident = 0;
        foreach (Node child in _map.GetChildren(true))
        {
            if (child is AreaRoot)
            {
                resident++;
            }
        }

        long mem = (long)OS.GetStaticMemoryUsage() / (1024 * 1024);

        long surfaces = 0;
        foreach (Node child in _map.GetChildren(true))
        {
            if (child is AreaRoot tile &&
                tile.GetNodeOrNull<Node3D>(AreaRoot.ChunksNode)?.GetChildOrNull<MeshInstance3D>(0)?.Mesh
                    is ArrayMesh mesh)
            {
                surfaces += mesh.GetSurfaceCount();
            }
        }

        GD.Print($"  [stream] resident {_map.ResidentTiles} tiles / {_map.ResidentChunks} chunks, " +
                 $"{_map.BuildingTiles} building, catalogue detail+low listed");
        _log.Add($"step {_step}: camera ({_camera.Position.X:F0},{_camera.Position.Z:F0}) -> " +
                 $"{resident} tiles, {surfaces} surfaces, {mem} MB total | " +
                 $"maps ~{resident * 1734 / 1024} MB, {TerrainSplat.LayerTextureCount} layer textures " +
                 $"~{TerrainSplat.LayerTextureBytes / (1024 * 1024)} MB");
        GD.Print(_log[^1]);

        _step++;
        if (_step > 6)
        {
            GD.Print("--- streaming summary ---");
            foreach (string line in _log)
            {
                GD.Print("  " + line);
            }

            _frameMs.Sort();
            int n = _frameMs.Count;
            double Pct(double p) => n == 0 ? 0 : _frameMs[Math.Min(n - 1, (int)(n * p))];
            int over16 = _frameMs.Count(ms => ms > 16.7);
            int over50 = _frameMs.Count(ms => ms > 50.0);
            GD.Print($"FRAME TIMES over {n} frames: median {Pct(0.50):F1} ms, p95 {Pct(0.95):F1} ms, " +
                     $"p99 {Pct(0.99):F1} ms, worst {(n > 0 ? _frameMs[n - 1] : 0):F1} ms");
            GD.Print($"  frames over 16.7 ms: {over16} ({100.0 * over16 / Math.Max(n, 1):F1}%), " +
                     $"over 50 ms: {over50} ({100.0 * over50 / Math.Max(n, 1):F1}%)");
            double Ms(long ticks) => ticks * 1000.0 / System.Diagnostics.Stopwatch.Frequency;
            GD.Print($"  spikes over 40 ms ({_spikes.Count}):");
            foreach (string spike in _spikes.Take(25))
            {
                GD.Print(spike);
            }

            GD.Print($"  skies: {MapRoot.SkiesBuilt} built, " +
                     $"{MapRoot.SkyBuildTicks * 1000.0 / System.Diagnostics.Stopwatch.Frequency:F0} ms total (on workers)");


            Quit();
            return true;
        }

        if (!_smooth)
        {
            _camera.Position += new Vector3(AreaTerrain.TileSize * 4.0f, 0.0f, 0.0f);
        }

        return false;
    }
}
