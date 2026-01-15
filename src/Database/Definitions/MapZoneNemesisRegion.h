#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct MapZoneNemesisRegion
    {
        uint32_t ID;
        uint32_t mapZoneHexGroupId;
        uint32_t localizedTextIdDescription;
        uint32_t faction2Id;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            mapZoneHexGroupId = file.getUint(recordIndex, i++);
            localizedTextIdDescription = file.getUint(recordIndex, i++);
            faction2Id = file.getUint(recordIndex, i++);
        }
    };
}
