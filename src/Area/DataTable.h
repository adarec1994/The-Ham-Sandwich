#pragma once

#include <vector>
#include <string>
#include <memory>
#include <map>
#include <algorithm>
#include <sstream>
#include "../Archive.h"
#include <cwctype>
#include "../Utils/BinStream.h"

class DataTable
{
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

    DTBLHeader mHeader;
    std::vector<FieldDescEntry> mFieldDescs;
    std::wstring mTableName;
    std::vector<int32> mIDLookup;
    std::vector<std::wstring> mColumnHeaders;
    std::wstring mFileName;
    std::unique_ptr<BinStream> mStream;
    std::vector<uint8> mContent;
    std::map<uint32, uint32> mRows;
    ArchivePtr mArchive;

    static std::wstring escapeJsonString(const std::wstring& input) {
       std::wostringstream ss;
       for (wchar_t ch : input) {
          if (!std::iswalnum(ch) && !std::iswprint(ch)) {
             continue;
          }

          switch (ch) {
          case L'\\': ss << L"\\\\"; break;
          case L'"': ss << L"\\\""; break;
          case L'/': ss << L"\\/"; break;
          case L'\b': ss << L"\\b"; break;
          case L'\f': ss << L"\\f"; break;
          case L'\n': ss << L"\\n"; break;
          case L'\r': ss << L"\\r"; break;
          case L'\t': ss << L"\\t"; break;
          default: ss << ch; break;
          }
       }
       return ss.str();
    }

    static std::wstring escapeSql(const std::wstring& input) {
       std::wostringstream ss;
       for (wchar_t ch : input) {
          if (!std::iswalnum(ch) && !std::iswprint(ch)) {
             continue;
          }

          switch (ch) {
          case L'\\': ss << L"\\\\"; break;
          case L'"': ss << L"'"; break;
          case L'/': ss << L"\\/"; break;
          case L'\b': ss << L"\\b"; break;
          case L'\f': ss << L"\\f"; break;
          case L'\n': ss << L"\\n"; break;
          case L'\r': ss << L"\\r"; break;
          case L'\t': ss << L"\\t"; break;
          default: ss << ch; break;
          }
       }
       return ss.str();
    }

public:
   DataTable(const FileEntryPtr& file, ArchivePtr archive)
           : mHeader{}, mArchive(std::move(archive))
   {
      if (mArchive && file) {
         mArchive->getFileData(file, mContent);
         mStream = std::make_unique<BinStream>(mContent);
      }
   }

    void exportAsSql(const std::wstring& filePath);

    bool initialLoad() {
       // Check if stream was successfully created in constructor
       if (!mStream) {
           return false;
       }

       mHeader = mStream->read<DTBLHeader>();

       mStream->seek(0x60 + mHeader.ofsFieldDesc);
       mFieldDescs.resize(static_cast<uint32>(mHeader.numRows));

       mStream->read(mFieldDescs.data(), sizeof(FieldDescEntry) * mHeader.numRows);

       std::vector<wchar_t> tableName(static_cast<uint32>(mHeader.lenTableName));
       mStream->seek(0x60);
       mStream->read(tableName.data(), tableName.size() * sizeof(wchar_t));
       tableName.push_back(static_cast<wchar_t>(0));

       mTableName = tableName.data();

       uint64 offset = mFieldDescs.size() * sizeof(FieldDescEntry) + mHeader.ofsFieldDesc + 0x60;
       if (offset % 16) {
          offset += 16 - (offset % 16);
       }

       uint32 size = 0;

       for (uint32 i = 0; i < mHeader.numRows; ++i) {
          switch (mFieldDescs[i].type) {
          case FieldType::Float:
          case FieldType::UInt32:
          case FieldType::Bool:
             size += 4;
             break;

          case FieldType::UInt64:
          case FieldType::StringTableOffset:
             size += 8;
             break;
          default:
             break;
          }

          auto* title = reinterpret_cast<wchar_t*>(mContent.data() + offset + mFieldDescs[i].ofsFieldTitleTable);
          mColumnHeaders.emplace_back(title);
       }

       return true;
    }

    bool initialLoadIDs() {
       if (!initialLoad()) {
          return false;
       }

       mStream->seek(mHeader.ofsEntries + 0x60);
       for (uint32 i = 0; i < mHeader.numEntries; ++i) {
          mStream->seek(mHeader.ofsEntries + 0x60 + i * mHeader.recordSize);
          auto id = mStream->read<uint32>();
          mRows[id] = i;
       }

       return true;
    }

    template<typename T>
    bool getRowByIndex(uint32 index, T& row) {
       if (index >= mHeader.numEntries) {
          return false;
       }

       mStream->seek(index * mHeader.recordSize + 0x60 + mHeader.ofsEntries);
       std::vector<uint8> data(static_cast<std::size_t>(mHeader.recordSize));
       mStream->read(data.data(), data.size());

       bool skip = false;
       auto* outPtr = reinterpret_cast<uint8*>(&row);
       uint32 curOffset = 0;
       uint8* ptr = data.data();

       for (uint32 j = 0; j < mFieldDescs.size(); ++j) {
          if (skip && (j > 0 && mFieldDescs[j - 1].type == FieldType::StringTableOffset) && mFieldDescs[j].type != FieldType::StringTableOffset) {
             ptr += 4;
          }

          switch (mFieldDescs[j].type) {
          case FieldType::UInt32:
             if (curOffset + 4 > sizeof(T)) {
                return false;
             }

             *reinterpret_cast<uint32*>(outPtr + curOffset) = *reinterpret_cast<uint32*>(ptr);
             curOffset += sizeof(uint32);
             ptr += sizeof(uint32);
             break;

          case FieldType::UInt64:
             if (curOffset + 8 > sizeof(T)) {
                return false;
             }

             *reinterpret_cast<uint64*>(outPtr + curOffset) = *reinterpret_cast<uint64*>(ptr);
             curOffset += sizeof(uint64);
             ptr += sizeof(uint64);
             break;

          case FieldType::Float:
             if (curOffset + 4 > sizeof(T)) {
                return false;
             }

             *reinterpret_cast<float*>(outPtr + curOffset) = *reinterpret_cast<float*>(ptr);
             curOffset += sizeof(float);
             ptr += sizeof(float);
             break;

          case FieldType::Bool:
             if (curOffset + 1 > sizeof(T)) {
                return false;
             }
             *reinterpret_cast<bool*>(outPtr + curOffset) = (*reinterpret_cast<uint32*>(ptr)) != 0;
             curOffset += sizeof(bool);
             ptr += 4;
             break;

          case FieldType::StringTableOffset:
          {
             if (curOffset + sizeof(wchar_t*) > sizeof(T)) {
                return false;
             }

             uint32 ofsLower = *reinterpret_cast<uint32*>(ptr);
             ptr += 4;
             // Unused high bytes of offset?
             // uint64 offset = *reinterpret_cast<uint32*>(ptr);
             ptr += 4;

             skip = ofsLower == 0;

             uint64 offset = 0;
             if (ofsLower > 0) {
                offset = ofsLower;
             }

             offset += mHeader.ofsEntries + 0x60;

             if (offset < mContent.size()) {
                *reinterpret_cast<wchar_t**>(outPtr + curOffset) = reinterpret_cast<wchar_t*>(&mContent[static_cast<uint32>(offset)]);
             }
             else {
                *reinterpret_cast<wchar_t**>(outPtr + curOffset) = const_cast<wchar_t*>(L"");
             }

             curOffset += sizeof(wchar_t*);
          }
          break;
          default:
              break;
          }
       }

       return true;
    }

    template<typename T>
    bool getRowById(uint32 id, T& row) {
       auto itr = mRows.find(id);
       if (itr == mRows.end()) {
          return false;
       }

       mStream->seek(itr->second * mHeader.recordSize + 0x60 + mHeader.ofsEntries);
       std::vector<uint8> data(static_cast<std::size_t>(mHeader.recordSize));
       mStream->read(data.data(), data.size());

       bool skip = false;
       auto* outPtr = reinterpret_cast<uint8*>(&row);
       uint32 curOffset = 0;
       uint8* ptr = data.data();

       for (uint32 j = 0; j < mFieldDescs.size(); ++j) {
          if (skip && (j > 0 && mFieldDescs[j - 1].type == FieldType::StringTableOffset) && mFieldDescs[j].type != FieldType::StringTableOffset) {
             ptr += 4;
          }

          switch (mFieldDescs[j].type) {
          case FieldType::UInt32:
             if (curOffset + 4 > sizeof(T)) {
                return false;
             }

             *reinterpret_cast<uint32*>(outPtr + curOffset) = *reinterpret_cast<uint32*>(ptr);
             curOffset += sizeof(uint32);
             ptr += sizeof(uint32);
             break;

          case FieldType::UInt64:
             if (curOffset + 8 > sizeof(T)) {
                return false;
             }

             *reinterpret_cast<uint64*>(outPtr + curOffset) = *reinterpret_cast<uint64*>(ptr);
             curOffset += sizeof(uint64);
             ptr += sizeof(uint64);
             break;

          case FieldType::Float:
             if (curOffset + 4 > sizeof(T)) {
                return false;
             }

             *reinterpret_cast<float*>(outPtr + curOffset) = *reinterpret_cast<float*>(ptr);
             curOffset += sizeof(float);
             ptr += sizeof(float);
             break;

          case FieldType::Bool:
             if (curOffset + 1 > sizeof(T)) {
                return false;
             }
             *reinterpret_cast<bool*>(outPtr + curOffset) = (*reinterpret_cast<uint32*>(ptr)) != 0;
             curOffset += sizeof(bool);
             ptr += 4;
             break;

          case FieldType::StringTableOffset:
          {
             if (curOffset + sizeof(wchar_t*) > sizeof(T)) {
                return false;
             }

             uint32 ofsLower = *reinterpret_cast<uint32*>(ptr);
             ptr += 4;
             // uint64 offset = *reinterpret_cast<uint32*>(ptr);
             ptr += 4;

             skip = ofsLower == 0;

             uint64 offset = 0;
             if (ofsLower > 0) {
                offset = ofsLower;
             }

             offset += mHeader.ofsEntries + 0x60;

             if (offset < mContent.size()) {
                *reinterpret_cast<wchar_t**>(outPtr + curOffset) = reinterpret_cast<wchar_t*>(&mContent[static_cast<uint32>(offset)]);
             }
             else {
                *reinterpret_cast<wchar_t**>(outPtr + curOffset) = const_cast<wchar_t*>(L"");
             }

             curOffset += sizeof(wchar_t*);
          }
          break;
          default:
              break;
          }
       }

       return true;
    }

    std::wstring loadRange(uint32 start, uint32 end) {
       if (start > mHeader.numEntries || start >= end) {
          return L"[ ]";
       }

       end = std::min(end, static_cast<uint32>(mHeader.numEntries));
       uint32 numElems = end - start;

       mStream->seek(mHeader.ofsEntries + 0x60 + start * mHeader.recordSize);
       std::vector<uint8> dataBuffer(static_cast<uint32>(mHeader.recordSize));
       std::wstringstream strm;
       strm << L"[ ";

       bool first = true;

       for (uint32 i = 0; i < numElems; ++i) {
          if (first) {
             first = false;
          }
          else {
             strm << L", ";
          }

          strm << L"{ ";

          mStream->read(dataBuffer.data(), dataBuffer.size());
          uint8* ptr = dataBuffer.data();

          bool innerFirst = true;
          bool skip = false;

          for (uint32 j = 0; j < mFieldDescs.size(); ++j) {
             if (skip && (j > 0 && mFieldDescs[j - 1].type == FieldType::StringTableOffset) && mFieldDescs[j].type != FieldType::StringTableOffset) {
                ptr += 4;
             }

             if (innerFirst) {
                innerFirst = false;
             }
             else {
                strm << L", ";
             }

             strm << L"\"" << mColumnHeaders[j] << L"\": ";
             switch (mFieldDescs[j].type) {
             case FieldType::UInt32:
                strm << *reinterpret_cast<uint32*>(ptr);
                ptr += sizeof(uint32);
                break;

             case FieldType::UInt64:
                strm << *reinterpret_cast<uint64*>(ptr);
                ptr += sizeof(uint64);
                break;

             case FieldType::Float:
                strm << *reinterpret_cast<float*>(ptr);
                ptr += sizeof(float);
                break;

             case FieldType::Bool:
                strm << ((*reinterpret_cast<uint32*>(ptr)) != 0 ? L"\"true\"" : L"\"false\"");
                ptr += 4;
                break;

             case FieldType::StringTableOffset:
             {
                uint32 ofsLower = *reinterpret_cast<uint32*>(ptr);
                ptr += 4;
                // uint64 offset = *reinterpret_cast<uint32*>(ptr);
                ptr += 4;

                skip = ofsLower == 0;

                uint64 offset = 0;
                if (ofsLower > 0) {
                   offset = ofsLower;
                }

                offset += mHeader.ofsEntries + 0x60;

                   std::wstring str;
                   if (offset < mContent.size()) {
                      str = reinterpret_cast<wchar_t*>(&mContent[static_cast<uint32>(offset)]);
                   }

                str = escapeJsonString(str);
                strm << L"\"" << str << L"\"";
             }
             break;
             default:
                 break;
             }
          }

          strm << L" }";
       }

       strm << L" ]";

       return strm.str();
    }

   [[nodiscard]] std::wstring createScheme() const {
       std::wstringstream strm;
       strm << L"[ { \"name\": \"" << mColumnHeaders[0] << L"\", \"label\": \"" << mColumnHeaders[0] << L"\", \"editable\": false, \"cell\": \"integer\" }";

       for (uint32 i = 1; i < mFieldDescs.size(); ++i) {
          strm << L", { \"name\": \"" << mColumnHeaders[i] << L"\", \"label\": \"" << mColumnHeaders[i] << L"\", \"editable\": false, \"cell\": \"";
          switch (mFieldDescs[i].type) {
          case FieldType::StringTableOffset:
          case FieldType::Bool:
             strm << L"string";
             break;

          case FieldType::UInt32:
          case FieldType::UInt64:
             strm << L"integer";
             break;

          case FieldType::Float:
             strm << L"number";
             break;
          default:
             strm << L"string";
             break;
          }

          strm << L"\" }";
       }

       strm << L" ]";

       return strm.str();
    }

    void exportAsCsv(const std::wstring& filePath);

   [[nodiscard]] uint32 numEntries() const { return static_cast<uint32>(mHeader.numEntries); }

   [[nodiscard]] const std::vector<std::wstring>& getColumnTitles() const { return mColumnHeaders; }
};

typedef std::shared_ptr<DataTable> DataTablePtr;