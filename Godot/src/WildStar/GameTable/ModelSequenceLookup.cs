using System.Collections.Generic;
using WildStar.Archive;

namespace WildStar.GameTable;

public static class ModelSequenceLookup
{
    private const string ModelSequenceTbl = "DB/ModelSequence.tbl";
    private const string DescriptionField = "description";

    private static Dictionary<uint, string>? _cache;

    public static void Clear() => _cache = null;

    public static Dictionary<uint, string> Load(WsFileSystem fs)
    {
        if (_cache is not null)
        {
            return _cache;
        }

        var names = new Dictionary<uint, string>();
        byte[]? data = ModelMeshLookup.FindTbl(fs, ModelSequenceTbl);

        if (data != null && TblReader.TryParse(data, out TblReader tbl, out _))
        {
            int field = tbl.IndexOfField(DescriptionField);
            if (field >= 0)
            {
                for (int row = 0; row < tbl.RecordCount; row++)
                {
                    string name = tbl.GetString(row, field);
                    if (name.Length != 0)
                    {
                        names[tbl.GetUInt(row, 0)] = name;
                    }
                }
            }
        }

        _cache = names;
        return names;
    }
}
