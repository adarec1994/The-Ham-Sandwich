#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct GossipEntry
    {
        uint32_t ID;
        uint32_t gossipSetId;
        uint32_t indexOrder;
        uint32_t localizedTextId;
        uint32_t prerequisiteId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            gossipSetId = file.getUint(recordIndex, i++);
            indexOrder = file.getUint(recordIndex, i++);
            localizedTextId = file.getUint(recordIndex, i++);
            prerequisiteId = file.getUint(recordIndex, i++);
        }
    };
}
