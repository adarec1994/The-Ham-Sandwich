#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TradeskillProficiency
    {
        static constexpr const char* GetFileName() { return "TradeskillProficiency"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t proficiencyFlagEnum;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            proficiencyFlagEnum = file.getUint(recordIndex, col++);
        }
    };
}