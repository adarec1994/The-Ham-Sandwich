#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct StoreKeyword
    {
        static constexpr const char* GetFileName() { return "StoreKeyword"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t storeDisplayInfoId;
        std::wstring keyword;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            storeDisplayInfoId = file.getUint(recordIndex, col++);
            keyword = file.getString(recordIndex, col++);
        }
    };
}