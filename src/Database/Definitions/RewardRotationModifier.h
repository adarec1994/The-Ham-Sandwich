#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct RewardRotationModifier
    {
        uint32_t ID;
        uint32_t rewardPropertyId;
        uint32_t rewardPropertyData;
        float value;
        uint32_t minPlayerLevel;
        uint32_t worldDifficultyFlags;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            rewardPropertyId = f.getUint(row, i++);
            rewardPropertyData = f.getUint(row, i++);
            value = f.getFloat(row, i++);
            minPlayerLevel = f.getUint(row, i++);
            worldDifficultyFlags = f.getUint(row, i++);
        }
    };
}
