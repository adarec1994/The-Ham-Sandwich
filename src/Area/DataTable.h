#pragma once

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <algorithm>
#include <sstream>
#include "../Archive.h"
#include "../Utils/BinStream.h"

class DataTable
{
#pragma pack(push, 1)
    struct DTBLHeader
    {
        uint32 magic;
        uint32 version;
        uint64 lenTableName;
        uint64 unk1;
        uint64 recordSize;
        uint64 numRows;
        uint64 ofsFieldDesc;
        uint64 numEntries;
        uint64 sizeEntryBlock;
        uint64 ofsEntries;
        uint64 maxEntry;
        uint64 ofsIDLookup;
        uint64 unk3Zero;
    };

    enum class FieldType : uint32
    {
        UInt32 = 3,
        Float = 4,
        Bool = 11,
        UInt64 = 20,
        StringTableOffset = 0x82,
        ForceDword = 0xFFFFFFFF
    };

    struct FieldDescEntry
    {
        uint64 unk1;
        uint64 ofsFieldTitleTable;
        FieldType type;
        uint32 unk6;
    };
#pragma pack(pop)

    DTBLHeader mHeader;
    std::vector<FieldDescEntry> mFieldDescs;
    std::wstring mTableName;
    std::map<uint32, uint32> mRows;
    std::vector<std::wstring> mColumnHeaders;

    std::shared_ptr<BinStream> mStream;
    std::vector<uint8> mContent;
    ArchivePtr mArchive;

public:
    DataTable(FileEntryPtr file, ArchivePtr archive);

    bool initialLoad();
    bool initialLoadIDs();

    template<typename T>
    bool getRowById(uint32 id, T& row)
    {
        auto itr = mRows.find(id);
        if (itr == mRows.end()) {
            return false;
        }

        mStream->seek(itr->second * mHeader.recordSize + 0x60 + mHeader.ofsEntries);

        std::vector<uint8> data((size_t)mHeader.recordSize);
        mStream->read(data.data(), data.size());

        bool skip = false;
        uint8* outPtr = (uint8*) &row;
        uint32 curOffset = 0;
        uint8* ptr = data.data();

        for (size_t j = 0; j < mFieldDescs.size(); ++j) {
            if (skip == true && (j > 0 && mFieldDescs[j - 1].type == FieldType::StringTableOffset) && mFieldDescs[j].type != FieldType::StringTableOffset) {
                ptr += 4;
            }
            skip = false;

            switch (mFieldDescs[j].type) {
            case FieldType::UInt32:
                if (curOffset + 4 > sizeof(T)) return false;
                *(uint32*)(outPtr + curOffset) = *(uint32*) ptr;
                curOffset += sizeof(uint32);
                ptr += sizeof(uint32);
                break;

            case FieldType::UInt64:
                if (curOffset + 8 > sizeof(T)) return false;
                *(uint64*) (outPtr + curOffset) = *(uint64*) ptr;
                curOffset += sizeof(uint64);
                ptr += sizeof(uint64);
                break;

            case FieldType::Float:
                if (curOffset + 4 > sizeof(T)) return false;
                *(float*) (outPtr + curOffset) = *(float*) ptr;
                curOffset += sizeof(float);
                ptr += sizeof(float);
                break;

            case FieldType::Bool:
                if (curOffset + 1 > sizeof(T)) return false;
                *(bool*) (outPtr + curOffset) = (*(uint32*) ptr) != 0;
                curOffset += sizeof(bool);
                ptr += 4;
                break;

            case FieldType::StringTableOffset:
                {
                    if (curOffset + sizeof(wchar_t*) > sizeof(T)) return false;

                    uint32 ofsLower = *(uint32*) ptr;
                    ptr += 4;
                    ptr += 4;

                    skip = (ofsLower == 0);

                    uint64 finalOffset = ofsLower;
                    if (finalOffset > 0) {
                        finalOffset += mHeader.ofsEntries + 0x60;
                    }

                    if (finalOffset > 0 && finalOffset < mContent.size()) {
                        *(wchar_t**)(outPtr + curOffset) = (wchar_t*) &mContent[(uint32) finalOffset];
                    } else {
                        *(wchar_t**) (outPtr + curOffset) = (wchar_t*)L"";
                    }

                    curOffset += sizeof(wchar_t*);
                }
                break;
            }
        }
        return true;
    }
};

typedef std::shared_ptr<DataTable> DataTablePtr;