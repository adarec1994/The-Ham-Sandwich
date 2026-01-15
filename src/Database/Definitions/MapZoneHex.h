#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct MapZoneHex
    {
        uint32_t ID;
        uint32_t mapZoneId;
        uint32_t pos0;
        uint32_t pos1;
        uint32_t flags;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            mapZoneId = file.getUint(recordIndex, i++);
            pos0 = file.getUint(recordIndex, i++);
            pos1 = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
        }
    };
}
