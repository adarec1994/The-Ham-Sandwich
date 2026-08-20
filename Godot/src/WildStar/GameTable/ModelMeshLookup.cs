using System;
using System.Collections.Generic;
using WildStar.Archive;

namespace WildStar.GameTable;

public static class ModelMeshLookup
{
    private const string Creature2ModelInfoTbl = "DB/Creature2ModelInfo.tbl";
    private const string Creature2DisplayInfoTbl = "DB/Creature2DisplayInfo.tbl";
    private const string ModelMeshTbl = "DB/ModelMesh.tbl";

    private const string AssetPathField = "assetPath";
    private const string MeshIdPrefix = "modelMeshId";
    private const int MeshIdSlots = 16;

    public static int[][] FindOutfitPresets(WsFileSystem fs, string modelPath)
    {
        string normalized = NormalizePath(modelPath);
        var presets = new List<int[]>();

        Collect(fs, Creature2ModelInfoTbl, normalized, presets);
        Collect(fs, Creature2DisplayInfoTbl, normalized, presets);

        return Deduplicate(presets);
    }

    private static void Collect(WsFileSystem fs, string tblPath, string normalized,
                                List<int[]> presets)
    {
        byte[]? data = FindTbl(fs, tblPath);
        if (data == null || !TblReader.TryParse(data, out TblReader tbl, out _))
        {
            return;
        }

        int pathField = tbl.IndexOfField(AssetPathField);
        if (pathField < 0)
        {
            return;
        }

        var meshFields = new List<int>(MeshIdSlots);
        for (int slot = 0; slot < MeshIdSlots; slot++)
        {
            int index = tbl.IndexOfField(MeshIdPrefix + slot.ToString("00"));
            if (index >= 0)
            {
                meshFields.Add(index);
            }
        }

        if (meshFields.Count == 0)
        {
            return;
        }

        for (int row = 0; row < tbl.RecordCount; row++)
        {
            if (!Matches(NormalizePath(tbl.GetString(row, pathField)), normalized))
            {
                continue;
            }

            var keys = new List<int>(meshFields.Count);
            foreach (int field in meshFields)
            {
                uint value = tbl.GetUInt(row, field);
                if (value != 0)
                {
                    keys.Add((int)value);
                }
            }

            if (keys.Count > 0)
            {
                presets.Add(keys.ToArray());
            }
        }
    }

    private static bool Matches(string path, string normalized)
    {
        if (path.Length == 0)
        {
            return false;
        }

        return path.EndsWith(normalized, StringComparison.OrdinalIgnoreCase) ||
               normalized.EndsWith(path, StringComparison.OrdinalIgnoreCase);
    }

    private static int[][] Deduplicate(List<int[]> presets)
    {
        var seen = new HashSet<string>(StringComparer.Ordinal);
        var unique = new List<int[]>();

        foreach (int[] preset in presets)
        {
            Array.Sort(preset);
            if (seen.Add(string.Join(",", preset)))
            {
                unique.Add(preset);
            }
        }

        return unique.ToArray();
    }

    public static Dictionary<uint, string> LoadMeshNames(WsFileSystem fs)
    {
        var names = new Dictionary<uint, string>();

        byte[]? data = FindTbl(fs, ModelMeshTbl);
        if (data == null || !TblReader.TryParse(data, out TblReader tbl, out _))
        {
            return names;
        }

        int nameField = tbl.IndexOfField("EnumName");
        if (nameField < 0)
        {
            return names;
        }

        for (int row = 0; row < tbl.RecordCount; row++)
        {
            string name = tbl.GetString(row, nameField);
            if (name.Length != 0)
            {
                names[tbl.GetUInt(row, 0)] = name;
            }
        }

        return names;
    }

    private static byte[]? FindTbl(WsFileSystem fs, string path)
    {
        foreach (WsArchive archive in fs.Archives)
        {
            if (archive.TryGetFile(path, out WsFile file))
            {
                try
                {
                    return file.ReadAllBytes();
                }
                catch
                {
                    continue;
                }
            }
        }

        return null;
    }

    private static string NormalizePath(string path) =>
        path.Replace('\\', '/').TrimStart('/');
}
