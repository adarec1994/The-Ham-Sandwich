using System;
using System.Collections.Generic;
using WildStar.Archive;

namespace WildStar.GameTable;

public static class ModelMeshLookup
{
    private const string Creature2ModelInfoTbl = "DB/Creature2ModelInfo.tbl";
    private const string Creature2DisplayInfoTbl = "DB/Creature2DisplayInfo.tbl";
    private const string ModelMeshTbl = "DB/ModelMesh.tbl";

    public readonly struct VariantSlot
    {
        public readonly int[] Keys;

        public VariantSlot(int[] keys) { Keys = keys; }
    }

    public static VariantSlot[] FindVariantSlots(WsFileSystem fs, string modelPath)
    {
        string normalized = NormalizePath(modelPath);

        var slots = new HashSet<int>[16];
        for (int i = 0; i < 16; i++)
            slots[i] = new HashSet<int>();

        CollectSlots(fs, Creature2ModelInfoTbl, 1, 41, 16, normalized, slots);
        CollectSlots(fs, Creature2DisplayInfoTbl, 2, 60, 16, normalized, slots);

        var result = new List<VariantSlot>();
        for (int s = 0; s < 16; s++)
        {
            slots[s].Remove(0);
            if (slots[s].Count == 0)
                continue;

            var keys = new List<int>(slots[s]);
            keys.Sort();
            result.Add(new VariantSlot(keys.ToArray()));
        }

        return result.ToArray();
    }

    private static void CollectSlots(WsFileSystem fs, string tblPath, int pathField,
                                     int meshIdStart, int meshIdCount,
                                     string normalized, HashSet<int>[] slots)
    {
        byte[]? data = FindTbl(fs, tblPath);
        if (data == null || !TblReader.TryParse(data, out TblReader tbl))
            return;

        for (int i = 0; i < tbl.RecordCount; i++)
        {
            string path = NormalizePath(tbl.GetString(i, pathField));
            if (!path.EndsWith(normalized, StringComparison.OrdinalIgnoreCase) &&
                !normalized.EndsWith(path, StringComparison.OrdinalIgnoreCase))
                continue;

            for (int f = 0; f < meshIdCount && (meshIdStart + f) < tbl.FieldCount; f++)
            {
                uint v = tbl.GetUInt(i, meshIdStart + f);
                if (v != 0 && f < 16)
                    slots[f].Add((int)v);
            }
        }
    }

    public static Dictionary<uint, string> LoadMeshNames(WsFileSystem fs)
    {
        var names = new Dictionary<uint, string>();
        byte[]? data = FindTbl(fs, ModelMeshTbl);
        if (data == null || !TblReader.TryParse(data, out TblReader tbl))
            return names;

        for (int i = 0; i < tbl.RecordCount; i++)
        {
            uint id = tbl.GetUInt(i, 0);
            string name = tbl.GetString(i, 1);
            if (!string.IsNullOrEmpty(name))
                names[id] = name;
        }

        return names;
    }

    private static byte[]? FindTbl(WsFileSystem fs, string path)
    {
        foreach (var arc in fs.Archives)
        {
            if (arc.TryGetFile(path, out var f))
            {
                try { return f.ReadAllBytes(); }
                catch { continue; }
            }
        }
        return null;
    }

    private static string NormalizePath(string path)
    {
        return path.Replace('\\', '/').TrimStart('/');
    }
}
