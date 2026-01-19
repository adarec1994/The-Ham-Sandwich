#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct AccountCurrencyType
    {
        uint32_t ID;
        uint32_t localizedTextId;
        std::wstring iconName;
        uint32_t accountItemId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextId = file.getUint(recordIndex, i++);
            iconName = file.getString(recordIndex, i++);
            accountItemId = file.getUint(recordIndex, i++);
        }
    };
}
