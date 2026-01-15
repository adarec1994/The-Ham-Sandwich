#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TradeskillCatalyst
    {
        static constexpr const char* GetFileName() { return "TradeskillCatalyst"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t tradeSkillId;
        uint32_t tier;
        uint32_t tradeskillCatalystEnum00;
        uint32_t tradeskillCatalystEnum01;
        uint32_t tradeskillCatalystEnum02;
        uint32_t tradeskillCatalystEnum03;
        uint32_t tradeskillCatalystEnum04;
        float value00;
        float value01;
        float value02;
        float value03;
        float value04;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            tradeSkillId = file.getUint(recordIndex, col++);
            tier = file.getUint(recordIndex, col++);
            tradeskillCatalystEnum00 = file.getUint(recordIndex, col++);
            tradeskillCatalystEnum01 = file.getUint(recordIndex, col++);
            tradeskillCatalystEnum02 = file.getUint(recordIndex, col++);
            tradeskillCatalystEnum03 = file.getUint(recordIndex, col++);
            tradeskillCatalystEnum04 = file.getUint(recordIndex, col++);
            value00 = file.getFloat(recordIndex, col++);
            value01 = file.getFloat(recordIndex, col++);
            value02 = file.getFloat(recordIndex, col++);
            value03 = file.getFloat(recordIndex, col++);
            value04 = file.getFloat(recordIndex, col++);
        }
    };
}