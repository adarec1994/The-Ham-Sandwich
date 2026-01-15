#include "Tbl.h"
#include <cstring>

namespace Tbl
{
    static inline uint16_t rd_u16(const uint8_t* p) { uint16_t v; std::memcpy(&v, p, 2); return v; }
    static inline int32_t rd_i32(const uint8_t* p) { int32_t v; std::memcpy(&v, p, 4); return v; }
    static inline uint32_t rd_u32(const uint8_t* p) { uint32_t v; std::memcpy(&v, p, 4); return v; }
    static inline int64_t rd_i64(const uint8_t* p) { int64_t v; std::memcpy(&v, p, 8); return v; }
    static inline float rd_f32(const uint8_t* p) { float v; std::memcpy(&v, p, 4); return v; }

    static std::wstring readWString(const uint8_t* p, size_t maxLen = 1024)
    {
        std::wstring result;
        for (size_t i = 0; i < maxLen; i++)
        {
            wchar_t c = static_cast<wchar_t>(rd_u16(p + i * 2));
            if (c == 0) break;
            result += c;
        }
        return result;
    }

    static size_t align8(size_t offset)
    {
        return (offset + 7) & ~static_cast<size_t>(7);
    }

    bool File::load(const uint8_t* data, size_t size)
    {
        if (!data || size < HEADER_SIZE) return false;

        mData.assign(data, data + size);
        const uint8_t* p = mData.data();

        mHeader.magic = rd_i32(p + 0);
        mHeader.version = rd_i32(p + 4);
        mHeader.tableNameLength = rd_u32(p + 8);
        mHeader.unk0 = rd_i64(p + 16);
        mHeader.recordSize = rd_u32(p + 24);
        mHeader.fieldCount = rd_i64(p + 32);
        mHeader.fieldOffset = rd_i64(p + 40);
        mHeader.recordCount = rd_u32(p + 48);
        mHeader.totalRecordSize = rd_i64(p + 56);
        mHeader.recordOffset = rd_i64(p + 64);
        mHeader.maxID = rd_i64(p + 72);
        mHeader.lookupOffset = rd_i64(p + 80);

        size_t tableNameOffset = HEADER_SIZE;
        if (tableNameOffset + mHeader.tableNameLength * 2 <= size)
        {
            mTableName = readWString(p + tableNameOffset, mHeader.tableNameLength);
        }

        mColumns.resize(static_cast<size_t>(mHeader.fieldCount));
        size_t columnDataStart = HEADER_SIZE + static_cast<size_t>(mHeader.fieldOffset);
        size_t columnNameBase = columnDataStart + static_cast<size_t>(mHeader.fieldCount) * 24;
        if (mHeader.fieldCount % 2 != 0) columnNameBase += 8;

        for (size_t i = 0; i < static_cast<size_t>(mHeader.fieldCount); ++i)
        {
            size_t colOffset = columnDataStart + i * 24;
            if (colOffset + 24 > size) break;

            Column& col = mColumns[i];
            col.nameLength = rd_u32(p + colOffset);
            col.unk0 = rd_u32(p + colOffset + 4);
            col.nameOffset = rd_i64(p + colOffset + 8);
            col.dataType = static_cast<DataType>(rd_u16(p + colOffset + 16));
            col.unk1 = rd_u32(p + colOffset + 20);

            size_t namePos = columnNameBase + static_cast<size_t>(col.nameOffset);
            if (namePos < size)
            {
                col.name = readWString(p + namePos, col.nameLength);
            }
        }

        mFieldOffsets.resize(mColumns.size());
        size_t currentOffset = 0;
        for (size_t i = 0; i < mColumns.size(); ++i)
        {
            if (mColumns[i].dataType == DataType::String)
            {
                currentOffset = align8(currentOffset);
                mFieldOffsets[i] = currentOffset;
                currentOffset += 8;
            }
            else if (mColumns[i].dataType == DataType::Ulong)
            {
                currentOffset = align8(currentOffset);
                mFieldOffsets[i] = currentOffset;
                currentOffset += 8;
            }
            else if (mColumns[i].dataType == DataType::Float)
            {
                mFieldOffsets[i] = currentOffset;
                currentOffset += 4;
            }
            else
            {
                mFieldOffsets[i] = currentOffset;
                currentOffset += 4;
            }
        }

        size_t lookupOffset = HEADER_SIZE + static_cast<size_t>(mHeader.lookupOffset);
        mLookup.resize(static_cast<size_t>(mHeader.maxID), -1);
        for (size_t i = 0; i < static_cast<size_t>(mHeader.maxID); ++i)
        {
            size_t pos = lookupOffset + i * 4;
            if (pos + 4 <= size)
            {
                mLookup[i] = rd_i32(p + pos);
            }
        }

        return true;
    }

    const uint8_t* File::getRecordPtr(uint32_t recordIndex) const
    {
        if (recordIndex >= mHeader.recordCount) return nullptr;
        size_t offset = HEADER_SIZE + static_cast<size_t>(mHeader.recordOffset) + recordIndex * mHeader.recordSize;
        if (offset >= mData.size()) return nullptr;
        return mData.data() + offset;
    }

    size_t File::getFieldOffset(size_t fieldIndex) const
    {
        if (fieldIndex >= mFieldOffsets.size()) return 0;
        return mFieldOffsets[fieldIndex];
    }

    int32_t File::getLookup(uint32_t id) const
    {
        if (id >= mLookup.size()) return -1;
        return mLookup[id];
    }

    uint32_t File::getUint(uint32_t recordIndex, size_t fieldIndex) const
    {
        const uint8_t* rec = getRecordPtr(recordIndex);
        if (!rec || fieldIndex >= mFieldOffsets.size()) return 0;
        return rd_u32(rec + mFieldOffsets[fieldIndex]);
    }

    float File::getFloat(uint32_t recordIndex, size_t fieldIndex) const
    {
        const uint8_t* rec = getRecordPtr(recordIndex);
        if (!rec || fieldIndex >= mFieldOffsets.size()) return 0.0f;
        return rd_f32(rec + mFieldOffsets[fieldIndex]);
    }

    int64_t File::getInt64(uint32_t recordIndex, size_t fieldIndex) const
    {
        const uint8_t* rec = getRecordPtr(recordIndex);
        if (!rec || fieldIndex >= mFieldOffsets.size()) return 0;
        return rd_i64(rec + mFieldOffsets[fieldIndex]);
    }

    std::wstring File::getString(uint32_t recordIndex, size_t fieldIndex) const
    {
        const uint8_t* rec = getRecordPtr(recordIndex);
        if (!rec || fieldIndex >= mFieldOffsets.size()) return L"";

        size_t recordStart = HEADER_SIZE + static_cast<size_t>(mHeader.recordOffset);
        int64_t strOffset = rd_i64(rec + mFieldOffsets[fieldIndex]);
        size_t absOffset = recordStart + static_cast<size_t>(strOffset);

        if (absOffset >= mData.size()) return L"";
        return readWString(mData.data() + absOffset);
    }
}