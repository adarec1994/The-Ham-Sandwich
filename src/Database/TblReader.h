#pragma once
#include "Tbl.h"
#include <unordered_map>
#include <vector>
#include <memory>

namespace Tbl
{
    template<typename T>
    class Table
    {
    public:
        bool load(const uint8_t* data, size_t size)
        {
            if (!mFile.load(data, size))
                return false;

            mRecords.clear();
            mRecords.reserve(mFile.getRecordCount());

            for (uint32_t i = 0; i < mFile.getRecordCount(); ++i)
            {
                T record{};
                record.Read(mFile, i);
                mRecords[record.GetID()] = std::move(record);
            }
            return true;
        }

        const T* get(uint32_t id) const
        {
            auto it = mRecords.find(id);
            return it != mRecords.end() ? &it->second : nullptr;
        }

        const std::unordered_map<uint32_t, T>& entries() const { return mRecords; }
        size_t size() const { return mRecords.size(); }
        const File& getFile() const { return mFile; }

    private:
        File mFile;
        std::unordered_map<uint32_t, T> mRecords;
    };
}