#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct ItemImbuementReward
    {
        uint32_t ID;
        uint32_t itemImbuementRewardTypeEnum;
        uint32_t rewardObjectId;
        uint32_t rewardValue;
        float rewardValueFloat;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            itemImbuementRewardTypeEnum = file.getUint(recordIndex, i++);
            rewardObjectId = file.getUint(recordIndex, i++);
            rewardValue = file.getUint(recordIndex, i++);
            rewardValueFloat = file.getFloat(recordIndex, i++);
        }
    };
}
