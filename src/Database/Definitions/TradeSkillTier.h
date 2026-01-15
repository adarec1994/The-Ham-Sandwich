#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TradeskillTier
    {
        static constexpr const char* GetFileName() { return "TradeskillTier"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t tradeSkillId;
        uint32_t tier;
        uint32_t requiredXp;
        uint32_t learnXp;
        uint32_t craftXp;
        uint32_t firstCraftXp;
        uint32_t questXp;
        uint32_t failXp;
        uint32_t itemLevelMin;
        uint32_t maxAdditives;
        uint64_t relearnCost;
        uint32_t achievementCategoryId;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            tradeSkillId = file.getUint(recordIndex, col++);
            tier = file.getUint(recordIndex, col++);
            requiredXp = file.getUint(recordIndex, col++);
            learnXp = file.getUint(recordIndex, col++);
            craftXp = file.getUint(recordIndex, col++);
            firstCraftXp = file.getUint(recordIndex, col++);
            questXp = file.getUint(recordIndex, col++);
            failXp = file.getUint(recordIndex, col++);
            itemLevelMin = file.getUint(recordIndex, col++);
            maxAdditives = file.getUint(recordIndex, col++);
            relearnCost = file.getInt64(recordIndex, col++);
            achievementCategoryId = file.getUint(recordIndex, col++);
        }
    };
}