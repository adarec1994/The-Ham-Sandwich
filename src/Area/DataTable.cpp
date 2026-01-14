#include "DataTable.h"
#include <fstream>
#include <filesystem>
#include <string>

void DataTable::exportAsSql(const std::wstring& filePath)
{
   std::wofstream os{std::filesystem::path(filePath)};

    os << L"CREATE TABLE " << mTableName << L"(";

    uint64 offset = mFieldDescs.size() * sizeof(FieldDescEntry) + mHeader.ofsFieldDesc + 0x60;
    if (offset % 16)
    {
       offset += 16 - (offset % 16);
    }

    for (uint32 i = 0; i < mHeader.numRows; ++i)
    {
       auto* title = reinterpret_cast<wchar_t*>(mContent.data() + offset + mFieldDescs[i].ofsFieldTitleTable);

       os << title << " ";

       switch (mFieldDescs[i].type)
       {
       case FieldType::Float:
          os << "FLOAT";
          break;
       case FieldType::UInt32:
       case FieldType::Bool:
       case FieldType::UInt64:
          os << "INTEGER";
          break;
       case FieldType::StringTableOffset:
          os << "VARCHAR(255)";
          break;
       default:
          os << "TEXT";
          break;
       }

       if (i == mHeader.numRows - 1)
          os << ");";
       else
          os << ", ";

       mColumnHeaders.emplace_back(title);
    }

    os << std::endl;

    mStream->seek(mHeader.ofsEntries + 0x60);

    std::vector<uint8> dataBuffer(static_cast<uint32>(mHeader.recordSize));

    for (uint32 i = 0; i < mHeader.numEntries; ++i)
    {
       mStream->read(dataBuffer.data(), dataBuffer.size());
       uint8* ptr = dataBuffer.data();

       bool skip = false;

       os << "INSERT INTO " << mTableName << " VALUES (";

       for (uint32 j = 0; j < mFieldDescs.size(); ++j)
       {
          if (skip && (j > 0 && mFieldDescs[j - 1].type == FieldType::StringTableOffset) && mFieldDescs[j].type != FieldType::StringTableOffset)
          {
             ptr += 4;
             skip = false;
          }
          else
          {
             skip = false;
          }

          if (j != 0)
          {
             os << L",";
          }

          switch (mFieldDescs[j].type)
          {
          case FieldType::UInt32:
             os << *reinterpret_cast<uint32*>(ptr);
             ptr += sizeof(uint32);
             break;

          case FieldType::UInt64:
             os << *reinterpret_cast<uint64*>(ptr);
             ptr += sizeof(uint64);
             break;

          case FieldType::Float:
             os << *reinterpret_cast<float*>(ptr);
             ptr += sizeof(float);
             break;

          case FieldType::Bool:
             os << ((*reinterpret_cast<uint32*>(ptr)) != 0 ? L"0" : L"1");
             ptr += 4;
             break;

          case FieldType::StringTableOffset:
          {
             uint32 ofsLower = *reinterpret_cast<uint32*>(ptr);
             ptr += 4;
             // uint64 highBits = *reinterpret_cast<uint32*>(ptr);
             ptr += 4;

             skip = ofsLower == 0;

             uint64 strOffset = 0;
             if (ofsLower > 0) {
                strOffset = ofsLower;
             }

             strOffset += mHeader.ofsEntries + 0x60;

             std::wstring str;
             if (strOffset < mContent.size()) {
                str = reinterpret_cast<wchar_t*>(&mContent[static_cast<uint32>(strOffset)]);
             }

             str = escapeSql(str);
             os << L"\"" << str << L"\"";
          }
          break;
          default:
              break;
          }
       }

       os << ");";
       os << std::endl;
    }

    os.close();
}

void DataTable::exportAsCsv(const std::wstring& filePath) {
   std::wofstream os{std::filesystem::path(filePath)};

    for (uint32 i = 0; i < mColumnHeaders.size(); ++i) {
       if (i != 0) {
          os << L";";
       }

       os << L"\"" << mColumnHeaders[i] << L"\"";
    }

    os << std::endl;

    mStream->seek(mHeader.ofsEntries + 0x60);

    std::vector<uint8> dataBuffer(static_cast<uint32>(mHeader.recordSize));

    for (uint32 i = 0; i < mHeader.numEntries; ++i) {
       mStream->read(dataBuffer.data(), dataBuffer.size());
       uint8* ptr = dataBuffer.data();

       bool skip = false;

       for (uint32 j = 0; j < mFieldDescs.size(); ++j) {
          if (skip && (j > 0 && mFieldDescs[j - 1].type == FieldType::StringTableOffset) && mFieldDescs[j].type != FieldType::StringTableOffset) {
             ptr += 4;
          }

          if (j != 0) {
             os << L";";
          }

          switch (mFieldDescs[j].type) {
          case FieldType::UInt32:
             os << *reinterpret_cast<uint32*>(ptr);
             ptr += sizeof(uint32);
             break;

          case FieldType::UInt64:
             os << *reinterpret_cast<uint64*>(ptr);
             ptr += sizeof(uint64);
             break;

          case FieldType::Float:
             os << *reinterpret_cast<float*>(ptr);
             ptr += sizeof(float);
             break;

          case FieldType::Bool:
             os << ((*reinterpret_cast<uint32*>(ptr)) != 0 ? L"0" : L"1");
             ptr += 4;
             break;

          case FieldType::StringTableOffset:
          {
             uint32 ofsLower = *reinterpret_cast<uint32*>(ptr);
             ptr += 4;
             // uint64 highBits = *reinterpret_cast<uint32*>(ptr);
             ptr += 4;

             skip = ofsLower == 0;

             uint64 strOffset = 0;
             if (ofsLower > 0) {
                strOffset = ofsLower;
             }

             strOffset += mHeader.ofsEntries + 0x60;

             std::wstring str;
             if (strOffset < mContent.size()) {
                str = reinterpret_cast<wchar_t*>(&mContent[static_cast<uint32>(strOffset)]);
             }

             str = escapeJsonString(str);
             os << L"\"" << str << L"\"";
          }
          break;
          default:
              break;
          }
       }

       os << std::endl;
    }

    os.close();
}