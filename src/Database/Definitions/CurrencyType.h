#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct CurrencyType
    {
        uint32_t ID;
        std::wstring description;
        uint32_t localizedTextId;
        std::wstring iconName;
        uint64_t capAmount;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            description = file.getString(recordIndex, i++);
            localizedTextId = file.getUint(recordIndex, i++);
            iconName = file.getString(recordIndex, i++);
            capAmount = static_cast<uint64_t>(file.getInt64(recordIndex, i++));
        }
    };
}
