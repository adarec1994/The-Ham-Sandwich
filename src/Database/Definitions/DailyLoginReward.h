#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct DailyLoginReward
    {
        uint32_t ID;
        uint32_t loginDay;
        uint32_t dailyLoginRewardTypeEnum;
        uint32_t rewardObjectValue;
        uint32_t dailyLoginRewardTierEnum;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            loginDay = file.getUint(recordIndex, i++);
            dailyLoginRewardTypeEnum = file.getUint(recordIndex, i++);
            rewardObjectValue = file.getUint(recordIndex, i++);
            dailyLoginRewardTierEnum = file.getUint(recordIndex, i++);
        }
    };
}
