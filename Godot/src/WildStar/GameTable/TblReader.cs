using System;
using System.Buffers.Binary;
using System.Collections.Generic;
using System.Text;

namespace WildStar.GameTable;

public sealed class TblReader
{
    public const uint Signature = 0x4454424C;
    private const int HeaderSize = 96;
    private const int FieldDescSize = 24;

    public int RecordCount { get; }
    public int FieldCount { get; }

    private readonly byte[] _data;
    private readonly int _recordSize;
    private readonly int _recordStart;
    private readonly int _stringTableStart;
    private readonly FieldDesc[] _fields;

    public enum FieldType : ushort
    {
        UInt = 3,
        Single = 4,
        Boolean = 11,
        ULong = 20,
        String = 130,
    }

    public readonly struct FieldDesc
    {
        public readonly FieldType Type;
        public readonly int RecordOffset;

        internal FieldDesc(FieldType type, int offset)
        {
            Type = type;
            RecordOffset = offset;
        }
    }

    public static bool TryParse(byte[] data, out TblReader reader)
    {
        reader = null!;
        if (data.Length < HeaderSize)
            return false;

        uint sig = BinaryPrimitives.ReadUInt32LittleEndian(data.AsSpan(0));
        if (sig != Signature)
            return false;

        reader = new TblReader(data);
        return true;
    }

    private TblReader(byte[] data)
    {
        _data = data;

        _recordSize = (int)BinaryPrimitives.ReadUInt64LittleEndian(data.AsSpan(24));
        FieldCount = (int)BinaryPrimitives.ReadUInt64LittleEndian(data.AsSpan(32));
        long fieldOffset = (long)BinaryPrimitives.ReadUInt64LittleEndian(data.AsSpan(40));
        RecordCount = (int)BinaryPrimitives.ReadUInt64LittleEndian(data.AsSpan(48));
        long totalRecordSize = (long)BinaryPrimitives.ReadUInt64LittleEndian(data.AsSpan(56));
        long recordOffset = (long)BinaryPrimitives.ReadUInt64LittleEndian(data.AsSpan(64));

        _recordStart = HeaderSize + (int)recordOffset;
        _stringTableStart = _recordStart + _recordSize * RecordCount;

        _fields = new FieldDesc[FieldCount];
        int fieldBase = HeaderSize + (int)fieldOffset;
        int byteOffset = 0;
        for (int i = 0; i < FieldCount; i++)
        {
            int at = fieldBase + i * FieldDescSize;
            var type = (FieldType)BinaryPrimitives.ReadUInt16LittleEndian(data.AsSpan(at + 16));
            _fields[i] = new FieldDesc(type, byteOffset);
            byteOffset += FieldSize(type);
        }
    }

    private static int FieldSize(FieldType type) => type switch
    {
        FieldType.UInt => 4,
        FieldType.Single => 4,
        FieldType.Boolean => 4,
        FieldType.ULong => 8,
        FieldType.String => 8,
        _ => 4,
    };

    public ReadOnlySpan<byte> Record(int index)
    {
        int start = _recordStart + index * _recordSize;
        return _data.AsSpan(start, _recordSize);
    }

    public uint GetUInt(int recordIndex, int fieldIndex)
    {
        var rec = Record(recordIndex);
        return BinaryPrimitives.ReadUInt32LittleEndian(rec[_fields[fieldIndex].RecordOffset..]);
    }

    public float GetSingle(int recordIndex, int fieldIndex)
    {
        var rec = Record(recordIndex);
        return BitConverter.ToSingle(rec[_fields[fieldIndex].RecordOffset..]);
    }

    public string GetString(int recordIndex, int fieldIndex)
    {
        var rec = Record(recordIndex);
        int off = _fields[fieldIndex].RecordOffset;
        uint offset1 = BinaryPrimitives.ReadUInt32LittleEndian(rec[off..]);
        uint offset2 = BinaryPrimitives.ReadUInt32LittleEndian(rec[(off + 4)..]);
        uint strOffset = Math.Max(offset1, offset2);
        if (strOffset == 0)
            return string.Empty;

        int tableOffset = (int)(strOffset - (uint)(_recordSize * RecordCount));
        int abs = _stringTableStart + tableOffset;
        if (abs < 0 || abs >= _data.Length)
            return string.Empty;

        int end = abs;
        while (end < _data.Length && _data[end] != 0)
            end++;

        return Encoding.UTF8.GetString(_data, abs, end - abs);
    }

    public FieldType GetFieldType(int fieldIndex) => _fields[fieldIndex].Type;
}
