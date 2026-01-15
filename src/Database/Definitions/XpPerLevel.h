#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct XpPerLevel
    {
        static constexpr const char* GetFileName() { return "XpPerLevel"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t minXpForLevel;
        uint32_t baseQuestXpPerLevel;
        uint32_t abilityPointsPerLevel;
        uint32_t attributePointsPerLevel;
        uint32_t baseRepRewardPerLevel;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            minXpForLevel = file.getUint(recordIndex, col++);
            baseQuestXpPerLevel = file.getUint(recordIndex, col++);
            abilityPointsPerLevel = file.getUint(recordIndex, col++);
            attributePointsPerLevel = file.getUint(recordIndex, col++);
            baseRepRewardPerLevel = file.getUint(recordIndex, col++);
        }
    };
}