#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct PathReward
    {
        uint32_t ID;
        uint32_t pathRewardTypeEnum;
        uint32_t objectId;
        uint32_t spell4Id;
        uint32_t item2Id;
        uint32_t quest2Id;
        uint32_t characterTitleId;
        uint32_t prerequisiteId;
        uint32_t count;
        uint32_t pathRewardFlags;
        uint32_t pathScientistScanBotProfileId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            pathRewardTypeEnum = file.getUint(recordIndex, i++);
            objectId = file.getUint(recordIndex, i++);
            spell4Id = file.getUint(recordIndex, i++);
            item2Id = file.getUint(recordIndex, i++);
            quest2Id = file.getUint(recordIndex, i++);
            characterTitleId = file.getUint(recordIndex, i++);
            prerequisiteId = file.getUint(recordIndex, i++);
            count = file.getUint(recordIndex, i++);
            pathRewardFlags = file.getUint(recordIndex, i++);
            pathScientistScanBotProfileId = file.getUint(recordIndex, i++);
        }
    };
}
