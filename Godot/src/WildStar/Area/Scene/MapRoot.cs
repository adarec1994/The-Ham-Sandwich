using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Godot;
using WildStar.Sky;

namespace WildStar.Area;

[Tool]
[GlobalClass]
public partial class MapRoot : Node3D
{
    [Export] public bool StreamDiagnostics { get; set; } = true;

    [Export(PropertyHint.Range, "16,500,1")]
    public double StreamSpikeMs { get; set; } = 50.0;

    private ulong _lastFrameUsec;

    private int _spikesLogged;

    private int _addedThisFrame, _freedThisFrame, _skiesThisFrame;

    private bool _appliedMixThisFrame;

    private ulong _addUsec, _assembleUsec;

    private int _chunksThisFrame;

    private readonly List<MapSceneBuilder.PendingTile> _showing = new();

    [Export(PropertyHint.Range, "1,256,1")]
    public int ChunksPerFrame { get; set; } =
        int.TryParse(System.Environment.GetEnvironmentVariable("WS_CHUNKS_PER_FRAME"), out int perFrame) && perFrame > 0
            ? perFrame
            : 16;

    [Export(PropertyHint.Range, "8,100,1")]
    public double FrameBudgetMs { get; set; } = 22.0;

    private double _chunkBudget = 4.0;

    [Export(PropertyHint.Range, "0,65536,512")]
    public float LowRadius { get; set; } = 8192.0f;

    [Export(PropertyHint.Range, "0,1,0.05")]
    public float LowFadeIn { get; set; } = 0.85f;

    private const double SampleInterval = 0.1;

    private const float MinSkyWeight = 0.02f;

    private const int DetachAfterSamples = 8;

    private const double ClockPushInterval = 0.1;

    private const float SwapMargin = 0.05f;

    [Export] public string MapName { get; set; } = string.Empty;

    [Export] public int FocusX { get; set; }

    [Export] public int FocusZ { get; set; }

    [Export] public int DetailRadius { get; set; } = 1;

    [Export] public int DetailTiles { get; set; }

    [Export] public int LowTiles { get; set; }

    [Export] public int SkyId { get; set; }

    [Export] public bool SkyFollowsCamera { get; set; } = true;

    private float _timeOfDay = 43200.0f;

    [Export(PropertyHint.Range, "0,86400,1")]
    public float TimeOfDay
    {
        get => _timeOfDay;
        set
        {
            _timeOfDay = ((value % 86400.0f) + 86400.0f) % 86400.0f;
            PushTime();
        }
    }

    [Export] public bool RunClock { get; set; } = true;

    [Export] public float DayLengthSeconds { get; set; } = SkyRoot.DefaultDayLengthSeconds;

    [Export] public bool HoldSkyOverNoData { get; set; } = true;

    private float _detailRange;

    [Export(PropertyHint.Range, "0,40000,100")]
    public float DetailRange
    {
        get => _detailRange;
        set
        {
            _detailRange = Mathf.Max(value, 0.0f);
            ApplyDetailRange();
        }
    }

    [Export(PropertyHint.Range, "0,20000,128")]
    public float StreamRadius { get; set; }

    [Export(PropertyHint.Range, "0,4096,128")]
    public float StreamHysteresis { get; set; } = AreaTerrain.TileSize;

    private const double StreamInterval = 0.25;

    private const int AssembliesPerFrame = 2;

    private const int FreesPerFrame = 2;

    private static readonly int MaxBuildsInFlight =
        int.TryParse(System.Environment.GetEnvironmentVariable("WS_BUILDS_IN_FLIGHT"), out int configured) && configured > 0
            ? configured
            : Math.Max(4, System.Environment.ProcessorCount);

    public void ApplyDetailRange()
    {
        bool unlimited = _detailRange <= 0.0f;
        foreach (Node child in GetChildren(true))
        {
            if (child is not AreaRoot tile)
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
                if (c is not GeometryInstance3D instance)
                {
                    continue;
                }

                instance.VisibilityRangeEnd = unlimited ? 0.0f : _detailRange;
                instance.VisibilityRangeEndMargin = unlimited ? 0.0f : 100.0f;
                instance.VisibilityRangeFadeMode = GeometryInstance3D.VisibilityRangeFadeModeEnum.Disabled;
            }
        }
    }

    private readonly Dictionary<(int X, int Z), AreaRoot> _tiles = new();

    private readonly Dictionary<string, SkyRoot> _skies = new(StringComparer.OrdinalIgnoreCase);

    private readonly HashSet<uint> _unresolvedLogged = new();

    private double _sinceSample;

    private string _activeSkyPath = string.Empty;

    private string _lastMix = string.Empty;

    private string _appliedMix = string.Empty;

    private readonly Dictionary<SkyRoot, int> _absent = new();

    private double _sinceClockPush;

    private SkyRoot? _activeSky;

    private sealed class LoadedTile
    {
        public LoadedTile(AreaRoot root, List<MapSceneBuilder.ChunkNode> chunks)
        {
            Root = root;
            Chunks = chunks;
        }

        public AreaRoot Root { get; }

        public List<MapSceneBuilder.ChunkNode> Chunks { get; }

        public int Admitted { get; set; }
    }

    private readonly List<MapSceneBuilder.TileEntry> _detail = new();

    private readonly List<MapSceneBuilder.TileEntry> _low = new();

    private readonly Dictionary<(int X, int Z, bool Low), LoadedTile> _loaded = new();

    private readonly Dictionary<(int X, int Z, bool Low), Task<MapSceneBuilder.PendingTile?>> _building = new();

    private readonly List<LoadedTile> _admitting = new();

    private readonly List<(int X, int Z, bool Low)> _drop = new();

    private bool _catalogued;

    public int ResidentTiles => _loaded.Count;

    public int BuildingTiles => _building.Count;

    public int ResidentChunks
    {
        get
        {
            int n = 0;
            foreach (LoadedTile t in _loaded.Values)
            {
                n += t.Admitted;
            }

            return n;
        }
    }

    private static readonly System.Threading.SemaphoreSlim SkyGate = new(1, 1);

    private double _sinceStream = StreamInterval;

    private readonly List<(int X, int Z)> _pendingDrop = new();

    private bool _catalogueFailed;

    public override void _Ready()
    {
        _tiles.Clear();
        foreach (Node child in GetChildren(true))
        {
            if (child is AreaRoot tile && !tile.LowDetail)
            {
                _tiles[(tile.TileX, tile.TileZ)] = tile;
            }
        }

        if (GetNodeOrNull<SkyRoot>(AreaRoot.SkyNode) is SkyRoot initial)
        {
            _activeSky = initial;
            initial.RunClock = false;
            initial.TimeOfDay = _timeOfDay;
            _activeSkyPath = AreaTables.SkyPath((uint)SkyId) ?? string.Empty;
            if (_activeSkyPath.Length > 0)
            {
                _skies[_activeSkyPath] = initial;
            }
        }
        else if (AreaTables.SkyPath(AreaTables.DefaultSkyId) is string fallback &&
                 AreaSceneBuilder.BuildSky(fallback) is SkyRoot built)
        {
            built.Name = AreaRoot.SkyNode;
            AddChild(built);
            built.RunClock = false;
            built.TimeOfDay = _timeOfDay;
            _activeSky = built;
            _activeSkyPath = fallback;
            _skies[fallback] = built;
            SkyId = (int)AreaTables.DefaultSkyId;
        }

        SetProcess(true);
    }

    public override void _Process(double delta)
    {
        ReportSpike();

        bool ticked = false;
        if (RunClock && DayLengthSeconds > 0.0f)
        {
            _timeOfDay = (_timeOfDay + (float)delta * (SkyFile.SecondsPerDay / DayLengthSeconds)) % 86400.0f;
            ticked = true;
        }

        Stream(delta);

        _sinceClockPush += delta;
        if (!Follow(delta) && ticked && _sinceClockPush >= ClockPushInterval)
        {
            _sinceClockPush = 0.0;
            PushTime();
        }
    }

    private void ReportSpike()
    {
        if (!StreamDiagnostics)
        {
            return;
        }

        ulong now = Time.GetTicksUsec();
        ulong previous = _lastFrameUsec;
        _lastFrameUsec = now;

        if (previous != 0)
        {
            double frameMs = (now - previous) / 1000.0;
            if (frameMs > FrameBudgetMs)
            {
                _chunkBudget = Math.Max(1.0, _chunkBudget * 0.5);
            }
            else if (_chunksThisFrame > 0 || _chunkBudget < 1.0)
            {
                _chunkBudget = Math.Min(ChunksPerFrame, _chunkBudget + 0.5);
            }
        }

        if (previous != 0 && _spikesLogged < 200)
        {
            double ms = (now - previous) / 1000.0;
            if (ms >= StreamSpikeMs)
            {
                _spikesLogged++;
                GD.Print($"[wildstar_mount] slow frame {ms:F0} ms " +
                         $"(AddChild {_addUsec / 1000.0:F0} ms, assemble {_assembleUsec / 1000.0:F0} ms, " +
                         $"rest {ms - (_addUsec + _assembleUsec) / 1000.0:F0} ms) — added {_addedThisFrame}, " +
                         $"chunks {_chunksThisFrame} (budget {_chunkBudget:F1}), " +
                         $"{_freedThisFrame}, skies attached {_skiesThisFrame}, " +
                         $"mix {(_appliedMixThisFrame ? "re-applied" : "unchanged")}; " +
                         $"{_tiles.Count} tiles resident, {_building.Count} building, " +
                         $"{_skyBuilding.Count} skies building, " +
                         $"gen2 {GC.CollectionCount(2)}, heap {GC.GetTotalMemory(false) / (1024 * 1024)} MB");
            }
        }

        _addUsec = 0;
        _assembleUsec = 0;
        _chunksThisFrame = 0;
        _addedThisFrame = 0;
        _freedThisFrame = 0;
        _skiesThisFrame = 0;
        _appliedMixThisFrame = false;
    }

    private void Stream(double delta)
    {
        if (StreamRadius <= 0.0f || _catalogueFailed)
        {
            return;
        }

        Harvest();

        _sinceStream += delta;
        if (_sinceStream < StreamInterval)
        {
            return;
        }

        _sinceStream = 0.0;

        Camera3D? camera = CurrentCamera();
        if (camera is null || !EnsureCatalogue())
        {
            return;
        }

        Vector3 eye = camera.GlobalPosition;
        RequestTiles(eye, _detail, StreamRadius, AreaTerrain.TileSize);
        RequestTiles(eye, _low, LowRadius, AreaTerrain.LowTileSize);
        Resolve(eye);
    }

    private void RequestTiles(Vector3 eye, List<MapSceneBuilder.TileEntry> tiles, float radius, float size)
    {
        foreach (MapSceneBuilder.TileEntry tile in tiles)
        {
            var key = (tile.Coord.X, tile.Coord.Z, tile.Coord.Low);
            if (_loaded.ContainsKey(key) || _building.ContainsKey(key) ||
                _building.Count >= MaxBuildsInFlight ||
                BoxDistance(tile.Coord.OriginX, tile.Coord.OriginZ, size, eye) > radius)
            {
                continue;
            }

            string map = MapName;
            MapSceneBuilder.TileEntry entry = tile;
            _building[key] = Task.Run(() => MapSceneBuilder.BuildTile(map, entry));
        }
    }

    private void Resolve(Vector3 eye)
    {
        float detailKeep = StreamRadius + AreaTerrain.ChunkSize;
        _drop.Clear();

        foreach (KeyValuePair<(int X, int Z, bool Low), LoadedTile> entry in _loaded)
        {
            LoadedTile tile = entry.Value;
            float size = entry.Key.Low ? AreaTerrain.LowChunkSize : AreaTerrain.ChunkSize;
            float radius = entry.Key.Low ? LowRadius : StreamRadius;
            bool anyNear = false;

            foreach (MapSceneBuilder.ChunkNode node in tile.Chunks)
            {
                float distance = BoxDistance(node.Centre.X - (size * 0.5f), node.Centre.Z - (size * 0.5f),
                                             size, eye);
                bool want = entry.Key.Low
                    ? distance <= radius && distance > StreamRadius * LowFadeIn
                    : distance <= radius;

                anyNear |= distance <= radius + size;
                if (node.Instance.Visible != want)
                {
                    node.Instance.Visible = want;
                }
            }

            if (!anyNear && BoxDistance(tile.Root.Position.X, tile.Root.Position.Z,
                                        entry.Key.Low ? AreaTerrain.LowTileSize : AreaTerrain.TileSize,
                                        eye) > radius + size)
            {
                _drop.Add(entry.Key);
            }
        }

        foreach ((int X, int Z, bool Low) key in _drop)
        {
            if (_loaded.Remove(key, out LoadedTile? leaving))
            {
                leaving.Root.QueueFree();
                if (!key.Low)
                {
                    _tiles.Remove((key.X, key.Z));
                }
            }
        }
    }

    private void Harvest()
    {
        HarvestSkies();

        List<(int X, int Z, bool Low)>? done = null;
        foreach (KeyValuePair<(int X, int Z, bool Low), Task<MapSceneBuilder.PendingTile?>> job in _building)
        {
            if (!job.Value.IsCompleted)
            {
                continue;
            }

            (done ??= new List<(int X, int Z, bool Low)>()).Add(job.Key);
            MapSceneBuilder.PendingTile? built =
                job.Value.IsCompletedSuccessfully ? job.Value.Result : null;
            if (built is null || _loaded.ContainsKey(job.Key))
            {
                built?.Root.QueueFree();
                continue;
            }

            AddChild(built.Root, false, InternalMode.Back);
            var loaded = new LoadedTile(built.Root, built.Chunks);
            _loaded[job.Key] = loaded;
            if (!job.Key.Low)
            {
                _tiles[(job.Key.X, job.Key.Z)] = built.Root;
            }

            _admitting.Add(loaded);
        }

        if (done is not null)
        {
            foreach ((int X, int Z, bool Low) key in done)
            {
                _building.Remove(key);
            }
        }

        int admitted = 0;
        int allowed = (int)_chunkBudget;
        while (_admitting.Count > 0 && admitted < allowed)
        {
            LoadedTile tile = _admitting[0];
            if (tile.Admitted >= tile.Chunks.Count || !GodotObject.IsInstanceValid(tile.Root))
            {
                _admitting.RemoveAt(0);
                continue;
            }

            MeshInstance3D instance = tile.Chunks[tile.Admitted].Instance;
            instance.Visible = false;
            tile.Root.AddChild(instance);
            tile.Admitted++;
            admitted++;
            _chunksThisFrame++;
        }
    }

    private static float BoxDistance(float originX, float originZ, float size, Vector3 point)
    {
        float dx = Mathf.Max(Mathf.Max(originX - point.X, point.X - (originX + size)), 0.0f);
        float dz = Mathf.Max(Mathf.Max(originZ - point.Z, point.Z - (originZ + size)), 0.0f);
        return Mathf.Sqrt((dx * dx) + (dz * dz));
    }

    private bool EnsureCatalogue()
    {
        if (_catalogued)
        {
            return true;
        }

        if (MapName.Length == 0)
        {
            GD.PushWarning("[wildstar_mount] cannot stream: the map root has no map name");
            _catalogueFailed = true;
            return false;
        }

        if (!MapSceneBuilder.TryListTiles(MapName, out List<MapSceneBuilder.TileEntry> tiles, out string error))
        {
            GD.PushWarning("[wildstar_mount] " + MapName + ": cannot stream (" + error + ")");
            _catalogueFailed = true;
            return false;
        }

        foreach (MapSceneBuilder.TileEntry tile in tiles)
        {
            (tile.Coord.Low ? _low : _detail).Add(tile);
        }

        _catalogued = true;
        return true;
    }

    private bool Follow(double delta)
    {
        _sinceSample += delta;
        if (_sinceSample < SampleInterval || !SkyFollowsCamera || _tiles.Count == 0)
        {
            return false;
        }

        _sinceSample = 0;

        Camera3D? camera = CurrentCamera();
        if (camera is null)
        {
            return false;
        }

        Span<uint> ids = stackalloc uint[AreaSky.MaxActive];
        Span<float> weights = stackalloc float[AreaSky.MaxActive];
        if (!SampleSky(camera.GlobalPosition, ids, weights))
        {
            return false;
        }

        for (int i = 0; i < AreaSky.MaxActive; i++)
        {
            if (ids[i] != 0 && AreaTables.SkyPath(ids[i]) is null)
            {
                if (_unresolvedLogged.Add(ids[i]))
                {
                    GD.Print($"[wildstar_mount] {MapName}: WorldSky {ids[i]} has no record; dropped from the sky mix (the client does the same)");
                }

                ids[i] = 0;
            }
        }

        if (HoldSkyOverNoData)
        {
            bool any = false;
            for (int i = 0; i < AreaSky.MaxActive; i++)
            {
                any |= ids[i] != 0;
            }

            if (!any)
            {
                return false;
            }
        }

        int active = AreaSky.Normalise(ids, weights);
        var mix = new List<(uint Id, float Weight, string Path)>(active);
        for (int i = 0; i < active; i++)
        {
            if (weights[i] >= MinSkyWeight && AreaTables.SkyPath(ids[i]) is string path)
            {
                mix.Add((ids[i], weights[i], path));
            }
        }

        if (mix.Count == 0)
        {
            return false;
        }

        var signature = new System.Text.StringBuilder(mix.Count * 12);
        foreach ((uint id, float weight, string _) in mix)
        {
            signature.Append(id).Append(':').Append((int)(weight * 64.0f)).Append('|');
        }

        string key = signature.ToString();
        if (string.Equals(key, _appliedMix, StringComparison.Ordinal))
        {
            return false;
        }

        _appliedMix = key;
        _appliedMixThisFrame = true;
        ApplyMix(mix, camera);
        return true;
    }

    private void PushTime()
    {
        foreach (Node child in GetChildren(true))
        {
            if (child is SkyRoot sky)
            {
                sky.RunClock = false;
                sky.TimeOfDay = _timeOfDay;
            }
        }
    }

    private Camera3D? CurrentCamera()
    {
#if TOOLS
        if (Engine.IsEditorHint())
        {
            return EditorInterface.Singleton.GetEditorViewport3D(0)?.GetCamera3D();
        }
#endif
        return GetViewport()?.GetCamera3D();
    }

    public bool SampleSky(Vector3 worldPosition, Span<uint> outIds, Span<float> outWeights)
    {
        Vector3 local = ToLocal(worldPosition);
        int tileX = (int)Math.Floor(local.X / AreaTerrain.TileSize) + AreaTerrain.TileOriginOffset;
        int tileZ = (int)Math.Floor(local.Z / AreaTerrain.TileSize) + AreaTerrain.TileOriginOffset;
        if (!_tiles.TryGetValue((tileX, tileZ), out AreaRoot? tile))
        {
            outIds.Clear();
            outWeights.Clear();
            return false;
        }

        float inTileX = local.X - (tileX - AreaTerrain.TileOriginOffset) * AreaTerrain.TileSize;
        float inTileZ = local.Z - (tileZ - AreaTerrain.TileOriginOffset) * AreaTerrain.TileSize;
        tile.SampleSky(inTileX, inTileZ, outIds, outWeights);
        return true;
    }

    public uint SkyIdAt(Vector3 worldPosition)
    {
        Span<uint> ids = stackalloc uint[AreaSky.MaxActive];
        Span<float> weights = stackalloc float[AreaSky.MaxActive];
        return SampleSky(worldPosition, ids, weights) ? ids[0] : 0u;
    }

    private void ApplyMix(List<(uint Id, float Weight, string Path)> mix, Camera3D camera)
    {
        (uint Id, float Weight, string Path) primary = mix[0];
        foreach ((uint id, float weight, string path) in mix)
        {
            if (string.Equals(path, _activeSkyPath, StringComparison.OrdinalIgnoreCase) &&
                weight + SwapMargin >= primary.Weight)
            {
                primary = (id, weight, path);
            }
        }

        SkyRoot? owner = SkyFor(primary.Path);
        if (owner is null)
        {
            return;
        }

        if (_activeSky is not null && !ReferenceEquals(_activeSky, owner) && GodotObject.IsInstanceValid(_activeSky))
        {
            _activeSky.Drive(_timeOfDay, 0.0f, false, null);
        }

        var sources = new List<(SkyFile Sky, float Weight)>(mix.Count);
        var attached = new List<(SkyRoot Sky, float Weight)>(mix.Count);
        Vector3 at = camera.GlobalPosition;
        foreach ((uint _, float weight, string path) in mix)
        {
            SkyRoot? sky = SkyFor(path);
            if (sky?.Sky is not SkyFile file)
            {
                continue;
            }

            sources.Add((file, weight));
            attached.Add((sky, weight));
            if (sky.GetParent() is null)
            {
                sky.Position = new Vector3(at.X, 0.0f, at.Z) - GlobalPosition;
                sky.EnvironmentEnabled = ReferenceEquals(sky, owner);
                AddChild(sky, false, InternalMode.Back);
                _skiesThisFrame++;
            }
        }

        foreach (SkyRoot cached in _skies.Values)
        {
            if (cached.GetParent() != this)
            {
                continue;
            }

            if (attached.Exists(a => ReferenceEquals(a.Sky, cached)))
            {
                _absent.Remove(cached);
                continue;
            }

            _absent.TryGetValue(cached, out int misses);
            if (misses + 1 < DetachAfterSamples)
            {
                _absent[cached] = misses + 1;
                cached.Drive(_timeOfDay, 0.0f, false, null);
                continue;
            }

            _absent.Remove(cached);
            cached.Drive(_timeOfDay, 0.0f, false, null);
            RemoveChild(cached);
        }

        foreach ((SkyRoot sky, float weight) in attached)
        {
            sky.Drive(_timeOfDay, weight, ReferenceEquals(sky, owner), sources);
        }

        string key = string.Join("|", mix.ConvertAll(m => m.Id.ToString()));
        if (!string.Equals(key, _lastMix, StringComparison.Ordinal))
        {
            _lastMix = key;
            GD.Print("[wildstar_mount] sky mix: " + Describe(mix));
        }

        _activeSky = owner;
        _activeSkyPath = primary.Path;
        SkyId = (int)primary.Id;
    }

    public int PreloadSkies(Action<string>? progress = null)
    {
        var paths = new List<string>();
        var seen = new HashSet<uint>();
        foreach (Node child in GetChildren())
        {
            if (child is not AreaRoot tile || tile.LowDetail)
            {
                continue;
            }

            foreach (int id in tile.ChunkSkyQuadrantIds)
            {
                if (id != 0 && seen.Add((uint)id) && AreaTables.SkyPath((uint)id) is string path &&
                    !_skies.ContainsKey(path))
                {
                    paths.Add(path);
                }
            }
        }

        int built = 0;
        foreach (string path in paths)
        {
            progress?.Invoke($"sky {++built}/{paths.Count}: {System.IO.Path.GetFileNameWithoutExtension(path.Replace('\\', '/'))}");
            SkyFor(path);
        }

        return built;
    }

    public static int SkiesBuilt;

    public static long SkyBuildTicks;

    private readonly Dictionary<string, Task<SkyRoot?>> _skyBuilding = new(StringComparer.OrdinalIgnoreCase);

    private SkyRoot? SkyFor(string path)
    {
        if (_skies.TryGetValue(path, out SkyRoot? sky))
        {
            return sky;
        }

        if (_skyBuilding.ContainsKey(path))
        {
            return null;
        }

        string name = "Sky_" + System.IO.Path.GetFileNameWithoutExtension(path.Replace('\\', '/'));
        _skyBuilding[path] = Task.Run(() =>
        {
            var clock = System.Diagnostics.Stopwatch.StartNew();
            SkyGate.Wait();
            try
            {
                SkyRoot? built = AreaSceneBuilder.BuildSky(path);
                if (built is not null)
                {
                    built.Name = name;
                }

                return built;
            }
            catch (Exception)
            {
                return null;
            }
            finally
            {
                SkyGate.Release();
                SkiesBuilt++;
                SkyBuildTicks += clock.ElapsedTicks;
            }
        });

        return null;
    }

    private void HarvestSkies()
    {
        if (_skyBuilding.Count == 0)
        {
            return;
        }

        List<string>? done = null;
        foreach (KeyValuePair<string, Task<SkyRoot?>> job in _skyBuilding)
        {
            if (!job.Value.IsCompleted)
            {
                continue;
            }

            (done ??= new List<string>()).Add(job.Key);
            SkyRoot? built = job.Value.IsCompletedSuccessfully ? job.Value.Result : null;
            if (built is not null && !_skies.ContainsKey(job.Key))
            {
                _skies[job.Key] = built;
            }
            else
            {
                built?.QueueFree();
            }
        }

        if (done is null)
        {
            return;
        }

        foreach (string path in done)
        {
            _skyBuilding.Remove(path);
        }

        _lastMix = string.Empty;
        _appliedMix = string.Empty;
    }

    private static string Describe(List<(uint Id, float Weight, string Path)> mix)
    {
        var parts = new List<string>(mix.Count);
        foreach ((uint id, float weight, string path) in mix)
        {
            parts.Add($"{System.IO.Path.GetFileNameWithoutExtension(path.Replace('\\', '/'))} ({id}) {weight:0.00}");
        }

        return string.Join(" + ", parts);
    }

    public override void _ExitTree()
    {
        foreach (SkyRoot sky in _skies.Values)
        {
            if (sky.GetParent() is null && GodotObject.IsInstanceValid(sky))
            {
                sky.QueueFree();
            }
        }

        _skies.Clear();
        _activeSky = null;
    }
}
