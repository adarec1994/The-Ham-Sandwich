#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct RewardRotationItem
    {
        uint32_t ID;
        uint32_t rewardItemTypeEnum;
        uint32_t rewardItemObject;
        uint32_t count;
        std::wstring iconPath;
        uint32_t minPlayerLevel;
        uint32_t worldDifficultyFlags;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            rewardItemTypeEnum = f.getUint(row, i++);
            rewardItemObject = f.getUint(row, i++);
            count = f.getUint(row, i++);
            iconPath = f.getString(row, i++);
            minPlayerLevel = f.getUint(row, i++);
            worldDifficultyFlags = f.getUint(row, i++);
        }
    };
}
