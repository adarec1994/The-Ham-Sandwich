#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct MapZoneHexGroupEntry
    {
        uint32_t ID;
        uint32_t mapZoneHexGroupId;
        uint32_t hexX;
        uint32_t hexY;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            mapZoneHexGroupId = file.getUint(recordIndex, i++);
            hexX = file.getUint(recordIndex, i++);
            hexY = file.getUint(recordIndex, i++);
        }
    };
}
