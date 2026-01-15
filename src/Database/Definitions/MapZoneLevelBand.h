#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct MapZoneLevelBand
    {
        uint32_t ID;
        uint32_t mapZoneHexGroupId;
        uint32_t levelMin;
        uint32_t levelMax;
        uint32_t labelX;
        uint32_t labelZ;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            mapZoneHexGroupId = file.getUint(recordIndex, i++);
            levelMin = file.getUint(recordIndex, i++);
            levelMax = file.getUint(recordIndex, i++);
            labelX = file.getUint(recordIndex, i++);
            labelZ = file.getUint(recordIndex, i++);
        }
    };
}
