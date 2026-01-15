#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TradeskillHarvestingInfo
    {
        static constexpr const char* GetFileName() { return "TradeskillHarvestingInfo"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t tradeSkillTierId;
        uint32_t prerequisiteId;
        uint32_t miniMapMarkerId;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            tradeSkillTierId = file.getUint(recordIndex, col++);
            prerequisiteId = file.getUint(recordIndex, col++);
            miniMapMarkerId = file.getUint(recordIndex, col++);
        }
    };
}