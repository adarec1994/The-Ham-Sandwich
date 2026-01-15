#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct MatchingRandomReward
    {
        uint32_t ID;
        uint32_t matchTypeEnum;
        uint32_t item2Id;
        uint32_t itemCount;
        uint32_t currencyTypeId;
        uint32_t currencyValue;
        uint32_t xpEarned;
        uint32_t item2IdLevelScale;
        uint32_t itemCountLevelScale;
        uint32_t currencyTypeIdLevelScale;
        uint32_t currencyValueLevelScale;
        uint32_t xpEarnedLevelScale;
        uint32_t worldDifficultyEnum;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            matchTypeEnum = file.getUint(recordIndex, i++);
            item2Id = file.getUint(recordIndex, i++);
            itemCount = file.getUint(recordIndex, i++);
            currencyTypeId = file.getUint(recordIndex, i++);
            currencyValue = file.getUint(recordIndex, i++);
            xpEarned = file.getUint(recordIndex, i++);
            item2IdLevelScale = file.getUint(recordIndex, i++);
            itemCountLevelScale = file.getUint(recordIndex, i++);
            currencyTypeIdLevelScale = file.getUint(recordIndex, i++);
            currencyValueLevelScale = file.getUint(recordIndex, i++);
            xpEarnedLevelScale = file.getUint(recordIndex, i++);
            worldDifficultyEnum = file.getUint(recordIndex, i++);
        }
    };
}
