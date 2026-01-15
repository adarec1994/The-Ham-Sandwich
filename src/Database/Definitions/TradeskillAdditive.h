#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TradeskillAdditive
    {
        static constexpr const char* GetFileName() { return "TradeskillAdditive"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t tradeSkillId;
        uint32_t tier;
        float vectorX;
        float vectorY;
        float radius;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            tradeSkillId = file.getUint(recordIndex, col++);
            tier = file.getUint(recordIndex, col++);
            vectorX = file.getFloat(recordIndex, col++);
            vectorY = file.getFloat(recordIndex, col++);
            radius = file.getFloat(recordIndex, col++);
        }
    };
}