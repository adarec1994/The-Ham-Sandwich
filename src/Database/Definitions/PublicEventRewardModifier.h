#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PublicEventRewardModifier
    {
        uint32_t ID;
        uint32_t publicEventId;
        uint32_t rewardPropertyId;
        uint32_t data;
        float offset;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            publicEventId = f.getUint(row, i++);
            rewardPropertyId = f.getUint(row, i++);
            data = f.getUint(row, i++);
            offset = f.getFloat(row, i++);
        }
    };
}
