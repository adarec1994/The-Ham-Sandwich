#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Tbl
{
    constexpr int HEADER_SIZE = 96;

    struct Header
    {
        int32_t magic;
        int32_t version;
        uint32_t tableNameLength;
        int64_t unk0;
        uint32_t recordSize;
        int64_t fieldCount;
        int64_t fieldOffset;
        uint32_t recordCount;
        int64_t totalRecordSize;
        int64_t recordOffset;
        int64_t maxID;
        int64_t lookupOffset;
    };

    enum class DataType : uint16_t
    {
        Uint = 3,
        Float = 4,
        Flags = 11,
        Ulong = 20,
        String = 130
    };

    struct Column
    {
        uint32_t nameLength;
        uint32_t unk0;
        int64_t nameOffset;
        DataType dataType;
        uint32_t unk1;
        std::wstring name;
    };

    class File
    {
    public:
        bool load(const uint8_t* data, size_t size);

        const Header& getHeader() const { return mHeader; }
        const std::wstring& getTableName() const { return mTableName; }
        const std::vector<Column>& getColumns() const { return mColumns; }

        uint32_t getUint(uint32_t recordIndex, size_t fieldIndex) const;
        float getFloat(uint32_t recordIndex, size_t fieldIndex) const;
        int64_t getInt64(uint32_t recordIndex, size_t fieldIndex) const;
        std::wstring getString(uint32_t recordIndex, size_t fieldIndex) const;

        size_t getFieldOffset(size_t fieldIndex) const;
        int32_t getLookup(uint32_t id) const;
        uint32_t getRecordCount() const { return mHeader.recordCount; }

    private:
        Header mHeader{};
        std::wstring mTableName;
        std::vector<Column> mColumns;
        std::vector<size_t> mFieldOffsets;
        std::vector<int32_t> mLookup;
        std::vector<uint8_t> mData;

        const uint8_t* getRecordPtr(uint32_t recordIndex) const;
    };
}