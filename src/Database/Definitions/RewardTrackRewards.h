#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct RewardTrackRewards
    {
        uint32_t ID;
        uint32_t rewardTrackId;
        uint32_t rewardPointFlags;
        uint32_t prerequisiteId;
        uint32_t flags;
        uint32_t currencyTypeId;
        uint32_t currencyAmount;
        uint32_t rewardTrackRewardTypeEnum00;
        uint32_t rewardTrackRewardTypeEnum01;
        uint32_t rewardTrackRewardTypeEnum02;
        uint32_t rewardChoiceId00;
        uint32_t rewardChoiceId01;
        uint32_t rewardChoiceId02;
        uint32_t rewardChoiceCount00;
        uint32_t rewardChoiceCount01;
        uint32_t rewardChoiceCount02;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            rewardTrackId = f.getUint(row, i++);
            rewardPointFlags = f.getUint(row, i++);
            prerequisiteId = f.getUint(row, i++);
            flags = f.getUint(row, i++);
            currencyTypeId = f.getUint(row, i++);
            currencyAmount = f.getUint(row, i++);
            rewardTrackRewardTypeEnum00 = f.getUint(row, i++);
            rewardTrackRewardTypeEnum01 = f.getUint(row, i++);
            rewardTrackRewardTypeEnum02 = f.getUint(row, i++);
            rewardChoiceId00 = f.getUint(row, i++);
            rewardChoiceId01 = f.getUint(row, i++);
            rewardChoiceId02 = f.getUint(row, i++);
            rewardChoiceCount00 = f.getUint(row, i++);
            rewardChoiceCount01 = f.getUint(row, i++);
            rewardChoiceCount02 = f.getUint(row, i++);
        }
    };
}
