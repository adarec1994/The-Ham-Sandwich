#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct Faction2Relationship
    {
        uint32_t ID;
        uint32_t factionId0;
        uint32_t factionId1;
        uint32_t factionLevel;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            factionId0 = file.getUint(recordIndex, i++);
            factionId1 = file.getUint(recordIndex, i++);
            factionLevel = file.getUint(recordIndex, i++);
        }
    };
}
