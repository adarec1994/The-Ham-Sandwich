using System;
using System.Collections.Generic;
using Godot;
using WildStar.Archive;
using WildStar.GameTable;

namespace WildStar.Area;

public readonly struct WorldLayerEntry
{
    public WorldLayerEntry(uint id, string diffusePath, string normalPath, float scaleU, float scaleV)
    {
        Id = id;
        DiffusePath = diffusePath;
        NormalPath = normalPath;
        ScaleU = scaleU;
        ScaleV = scaleV;
    }

    public uint Id { get; }

    public string DiffusePath { get; }

    public string NormalPath { get; }

    public float ScaleU { get; }

    public float ScaleV { get; }
}

public static class AreaTables
{
    private static Func<string, byte[]?>? _resolver;
    private static Func<WsFileSystem?>? _fileSystem;
    private static TblReader? _worldSky;
    private static Dictionary<uint, WorldLayerEntry>? _worldLayer;
    private static bool _tablesTried;
    private static readonly object TableGate = new();

    public static void SetResolver(Func<string, byte[]?>? resolver)
    {
        _resolver = resolver;
        _worldSky = null;
        _worldLayer = null;
        _tablesTried = false;
    }

    public static void SetFileSystem(Func<WsFileSystem?>? provider) => _fileSystem = provider;

    public static WsFileSystem? FileSystem => _fileSystem?.Invoke();

    public static byte[]? Read(string path) => _resolver?.Invoke(path.Replace('\\', '/'));

    private static void EnsureTables()
    {
        if (_tablesTried)
        {
            return;
        }

        lock (TableGate)
        {
            if (_tablesTried)
            {
                return;
            }

            byte[]? sky = Read("DB/WorldSky.tbl");
            if (sky is not null && TblReader.TryParse(sky, out TblReader skyTable, out _))
            {
                _worldSky = skyTable;
            }

            byte[]? layer = Read("DB/WorldLayer.tbl");
            if (layer is not null && TblReader.TryParse(layer, out TblReader layerTable, out _))
            {
                var entries = new Dictionary<uint, WorldLayerEntry>(layerTable.RecordCount);
                int colour = layerTable.IndexOfField("ColorMapPath");
                int normal = layerTable.IndexOfField("NormalMapPath");
                int meters = layerTable.IndexOfField("MetersPerTextureTile");
                for (int r = 0; r < layerTable.RecordCount; r++)
                {
                    uint id = layerTable.GetUInt(r, 0);
                    float scale = meters >= 0 ? layerTable.GetSingle(r, meters) : 1.0f;
                    entries[id] = new WorldLayerEntry(
                        id,
                        colour >= 0 ? layerTable.GetString(r, colour) : string.Empty,
                        normal >= 0 ? layerTable.GetString(r, normal) : string.Empty,
                        scale, scale);
                }

                _worldLayer = entries;
            }

            _tablesTried = true;
        }
    }

    public static bool TryWorldLayer(uint id, out WorldLayerEntry entry)
    {
        EnsureTables();
        if (_worldLayer is not null && _worldLayer.TryGetValue(id, out entry))
        {
            return true;
        }

        entry = default;
        return false;
    }

    public const uint DefaultSkyId = AreaSky.DefaultSkyId;

    public static string? ResolveSkyPath(uint id, out string label)
    {
        string? direct = SkyPath(id);
        if (direct is not null)
        {
            label = "WorldSky " + id;
            return direct;
        }

        label = "engine default (WorldSky " + id + " has no record)";
        return SkyPath(DefaultSkyId);
    }

    public static string? SkyPath(uint id)
    {
        if (id == 0)
        {
            return null;
        }

        EnsureTables();
        if (_worldSky is null)
        {
            return null;
        }

        int row = _worldSky.RecordIndexForId(id);
        if (row < 0)
        {
            return null;
        }

        int field = _worldSky.IndexOfField("assetPath");
        string path = field >= 0 ? _worldSky.GetString(row, field) : string.Empty;
        return path.Length > 0 ? path : null;
    }
}
