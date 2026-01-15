#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct StoreLink
    {
        static constexpr const char* GetFileName() { return "StoreLink"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        std::wstring enumName;
        uint32_t categoryData;
        uint32_t categoryDataPTR;
        uint32_t accountItemId;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            enumName = file.getString(recordIndex, col++);
            categoryData = file.getUint(recordIndex, col++);
            categoryDataPTR = file.getUint(recordIndex, col++);
            accountItemId = file.getUint(recordIndex, col++);
        }
    };
}