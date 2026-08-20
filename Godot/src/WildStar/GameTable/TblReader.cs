using System;
using System.Buffers.Binary;
using System.Text;

namespace WildStar.GameTable;

public sealed class TblReader
{
    public const uint Signature = 0x4454424C;
    public const int HeaderSize = 96;
    public const int FieldDescSize = 24;
    public const uint NoLookup = 0xFFFFFFFF;

    private const int HdrNameLength = 0x08;
    private const int HdrNameOffset = 0x10;
    private const int HdrRecordSize = 0x18;
    private const int HdrFieldCount = 0x20;
    private const int HdrFieldOffset = 0x28;
    private const int HdrRecordCount = 0x30;
    private const int HdrTotalData = 0x38;
    private const int HdrRecordOffset = 0x40;
    private const int HdrLookupCount = 0x48;
    private const int HdrLookupOffset = 0x50;

    private const int FieldDescNameLength = 0x00;
    private const int FieldDescNameOffset = 0x08;
    private const int FieldDescType = 0x10;
    private const int FieldDescFlags = 0x14;

    public enum FieldType : ushort
    {
        UInt = 3,
        Single = 4,
        Boolean = 11,
        Array = 128,
        ULong = 20,
        String = 130,
    }

    public readonly struct FieldDesc
    {
        public FieldDesc(string name, FieldType type, uint flags, int recordOffset, int size)
        {
            Name = name;
            Type = type;
            Flags = flags;
            RecordOffset = recordOffset;
            Size = size;
        }

        public string Name { get; }
        public FieldType Type { get; }
        public uint Flags { get; }
        public int RecordOffset { get; }
        public int Size { get; }
    }

    private readonly byte[] _data;
    private readonly int _recordStart;
    private readonly int _lookupStart;

    public string Name { get; }
    public int RecordCount { get; }
    public int RecordSize { get; }
    public int FieldCount => Fields.Length;
    public FieldDesc[] Fields { get; }
    public int LookupCount { get; }

    public static int SizeOf(FieldType type) => type switch
    {
        FieldType.UInt => 4,
        FieldType.Single => 4,
        FieldType.Boolean => 4,
        FieldType.ULong => 8,
        FieldType.String => 8,
        _ => 4,
    };

    public static bool TryParse(byte[] data, out TblReader reader, out string error)
    {
        reader = null!;

        if (data.Length < HeaderSize)
        {
            error = "shorter than a DTBL header";
            return false;
        }

        if (BinaryPrimitives.ReadUInt32LittleEndian(data) != Signature)
        {
            error = "missing DTBL signature";
            return false;
        }

        try
        {
            reader = new TblReader(data);
        }
        catch (Exception e)
        {
            error = e.Message;
            return false;
        }

        error = string.Empty;
        return true;
    }

    public static bool TryParse(byte[] data, out TblReader reader) =>
        TryParse(data, out reader, out _);

    private TblReader(byte[] data)
    {
        _data = data;

        int nameLength = (int)U64(HdrNameLength);
        long nameOffset = (long)U64(HdrNameOffset);
        RecordSize = (int)U64(HdrRecordSize);
        int fieldCount = (int)U64(HdrFieldCount);
        long fieldOffset = (long)U64(HdrFieldOffset);
        RecordCount = (int)U64(HdrRecordCount);
        long totalData = (long)U64(HdrTotalData);
        long recordOffset = (long)U64(HdrRecordOffset);
        LookupCount = (int)U64(HdrLookupCount);
        long lookupOffset = (long)U64(HdrLookupOffset);

        if (fieldCount < 0 || RecordCount < 0 || RecordSize <= 0)
        {
            throw new InvalidOperationException("implausible header counts");
        }

        Name = ReadWide(HeaderSize + nameOffset, nameLength);

        long fieldBase = HeaderSize + fieldOffset;
        long nameBase = fieldBase + AlignUp((long)fieldCount * FieldDescSize, 16);

        if (fieldBase < 0 || fieldBase + (long)fieldCount * FieldDescSize > data.Length)
        {
            throw new InvalidOperationException("field descriptors run past the end of the file");
        }

        _recordStart = checked((int)(HeaderSize + recordOffset));
        _lookupStart = checked((int)(HeaderSize + lookupOffset));

        if (_recordStart < 0 || _recordStart + totalData > data.Length)
        {
            throw new InvalidOperationException("record data runs past the end of the file");
        }

        if (LookupCount > 0 &&
            (_lookupStart < 0 || _lookupStart + (long)LookupCount * 4 > data.Length))
        {
            throw new InvalidOperationException("lookup table runs past the end of the file");
        }

        Fields = new FieldDesc[fieldCount];
        int offset = 0;

        for (int i = 0; i < fieldCount; i++)
        {
            long f = fieldBase + (long)i * FieldDescSize;
            int fieldNameLength = (int)BinaryPrimitives.ReadUInt32LittleEndian(
                data.AsSpan((int)(f + FieldDescNameLength)));
            long fieldNameOffset = (long)BinaryPrimitives.ReadUInt64LittleEndian(
                data.AsSpan((int)(f + FieldDescNameOffset)));
            var type = (FieldType)BinaryPrimitives.ReadUInt16LittleEndian(
                data.AsSpan((int)(f + FieldDescType)));
            uint flags = BinaryPrimitives.ReadUInt32LittleEndian(
                data.AsSpan((int)(f + FieldDescFlags)));

            int size = SizeOf(type);
            if (size == 8 && offset % 8 != 0)
            {
                offset += 8 - (offset % 8);
            }

            Fields[i] = new FieldDesc(ReadWide(nameBase + fieldNameOffset, fieldNameLength),
                                      type, flags, offset, size);
            offset += size;
        }
    }

    private static long AlignUp(long value, int to) => (value + to - 1) & ~((long)to - 1);

    private ulong U64(int at) => BinaryPrimitives.ReadUInt64LittleEndian(_data.AsSpan(at));

    private string ReadWide(long at, int chars)
    {
        if (at < 0 || chars <= 1 || at + (long)chars * 2 > _data.Length)
        {
            return string.Empty;
        }

        return Encoding.Unicode.GetString(_data, (int)at, (chars - 1) * 2);
    }

    public ReadOnlySpan<byte> Record(int index)
    {
        if (index < 0 || index >= RecordCount)
        {
            return ReadOnlySpan<byte>.Empty;
        }

        return _data.AsSpan(_recordStart + index * RecordSize, RecordSize);
    }

    public uint GetUInt(int recordIndex, int fieldIndex)
    {
        ReadOnlySpan<byte> record = Record(recordIndex);
        if (record.IsEmpty || (uint)fieldIndex >= (uint)Fields.Length)
        {
            return 0;
        }

        return BinaryPrimitives.ReadUInt32LittleEndian(record[Fields[fieldIndex].RecordOffset..]);
    }

    public ulong GetULong(int recordIndex, int fieldIndex)
    {
        ReadOnlySpan<byte> record = Record(recordIndex);
        if (record.IsEmpty || (uint)fieldIndex >= (uint)Fields.Length)
        {
            return 0;
        }

        return BinaryPrimitives.ReadUInt64LittleEndian(record[Fields[fieldIndex].RecordOffset..]);
    }

    public float GetSingle(int recordIndex, int fieldIndex)
    {
        ReadOnlySpan<byte> record = Record(recordIndex);
        if (record.IsEmpty || (uint)fieldIndex >= (uint)Fields.Length)
        {
            return 0.0f;
        }

        return BinaryPrimitives.ReadSingleLittleEndian(record[Fields[fieldIndex].RecordOffset..]);
    }

    public bool GetBool(int recordIndex, int fieldIndex) => GetUInt(recordIndex, fieldIndex) != 0;

    public string GetString(int recordIndex, int fieldIndex)
    {
        ReadOnlySpan<byte> record = Record(recordIndex);
        if (record.IsEmpty || (uint)fieldIndex >= (uint)Fields.Length)
        {
            return string.Empty;
        }

        ulong offset = BinaryPrimitives.ReadUInt64LittleEndian(
            record[Fields[fieldIndex].RecordOffset..]);
        if (offset == 0)
        {
            return string.Empty;
        }

        long at = _recordStart + (long)offset;
        if (at < 0 || at + 1 >= _data.Length)
        {
            return string.Empty;
        }

        int end = (int)at;
        while (end + 1 < _data.Length && (_data[end] != 0 || _data[end + 1] != 0))
        {
            end += 2;
        }

        return Encoding.Unicode.GetString(_data, (int)at, end - (int)at);
    }

    public string GetText(int recordIndex, int fieldIndex)
    {
        if ((uint)fieldIndex >= (uint)Fields.Length)
        {
            return string.Empty;
        }

        return Fields[fieldIndex].Type switch
        {
            FieldType.String => GetString(recordIndex, fieldIndex),
            FieldType.Single => GetSingle(recordIndex, fieldIndex).ToString("0.######"),
            FieldType.Boolean => GetBool(recordIndex, fieldIndex) ? "true" : "false",
            FieldType.ULong => GetULong(recordIndex, fieldIndex).ToString(),
            _ => GetUInt(recordIndex, fieldIndex).ToString(),
        };
    }

    public int IndexOfField(string name)
    {
        for (int i = 0; i < Fields.Length; i++)
        {
            if (string.Equals(Fields[i].Name, name, StringComparison.OrdinalIgnoreCase))
            {
                return i;
            }
        }

        return -1;
    }

    public int RecordIndexForId(uint id)
    {
        if (id >= (uint)LookupCount)
        {
            return -1;
        }

        uint index = BinaryPrimitives.ReadUInt32LittleEndian(
            _data.AsSpan(_lookupStart + (int)id * 4));

        return index == NoLookup || index >= (uint)RecordCount ? -1 : (int)index;
    }

    public FieldType GetFieldType(int fieldIndex) =>
        (uint)fieldIndex < (uint)Fields.Length ? Fields[fieldIndex].Type : FieldType.UInt;
}
