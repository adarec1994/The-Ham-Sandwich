#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct GossipSet
    {
        uint32_t ID;
        uint32_t flags;
        uint32_t gossipProximityEnum;
        uint32_t cooldown;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            gossipProximityEnum = file.getUint(recordIndex, i++);
            cooldown = file.getUint(recordIndex, i++);
        }
    };
}
