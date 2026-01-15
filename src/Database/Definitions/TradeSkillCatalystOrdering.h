#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TradeskillCatalystOrdering
    {
        static constexpr const char* GetFileName() { return "TradeskillCatalystOrdering"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t unlockLevel00;
        uint32_t unlockLevel01;
        uint32_t unlockLevel02;
        uint32_t unlockLevel03;
        uint32_t unlockLevel04;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            unlockLevel00 = file.getUint(recordIndex, col++);
            unlockLevel01 = file.getUint(recordIndex, col++);
            unlockLevel02 = file.getUint(recordIndex, col++);
            unlockLevel03 = file.getUint(recordIndex, col++);
            unlockLevel04 = file.getUint(recordIndex, col++);
        }
    };
}