#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct LiveEventDisplayItem
    {
        uint32_t ID;
        uint32_t liveEventId;
        uint32_t item2Id;
        uint32_t storeLinkId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            liveEventId = file.getUint(recordIndex, i++);
            item2Id = file.getUint(recordIndex, i++);
            storeLinkId = file.getUint(recordIndex, i++);
        }
    };
}
