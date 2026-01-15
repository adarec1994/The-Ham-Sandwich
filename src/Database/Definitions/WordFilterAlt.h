#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct WordFilterAlt
    {
        static constexpr const char* GetFileName() { return "WordFilterAlt"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        std::wstring filter;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            filter = file.getString(recordIndex, col++);
        }
    };
}