#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Quest2Reward
    {
        uint32_t ID;
        uint32_t quest2Id;
        uint32_t quest2RewardTypeId;
        uint32_t objectId;
        uint32_t objectAmount;
        uint32_t flags;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            quest2Id = f.getUint(row, i++);
            quest2RewardTypeId = f.getUint(row, i++);
            objectId = f.getUint(row, i++);
            objectAmount = f.getUint(row, i++);
            flags = f.getUint(row, i++);
        }
    };
}
