#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct HousingPropertyInfo
    {
        uint32_t ID;
        uint32_t localizedTextIdName;
        uint32_t worldId;
        uint32_t housingMapInfoId;
        uint32_t cost;
        uint32_t housingFacingEnum;
        uint32_t worldLocation2Id;
        uint32_t worldZoneId;
        uint32_t housingPropertyTypeId;
        uint32_t worldLayerIdDefault00;
        uint32_t worldLayerIdDefault01;
        uint32_t worldLayerIdDefault02;
        uint32_t worldLayerIdDefault03;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            worldId = file.getUint(recordIndex, i++);
            housingMapInfoId = file.getUint(recordIndex, i++);
            cost = file.getUint(recordIndex, i++);
            housingFacingEnum = file.getUint(recordIndex, i++);
            worldLocation2Id = file.getUint(recordIndex, i++);
            worldZoneId = file.getUint(recordIndex, i++);
            housingPropertyTypeId = file.getUint(recordIndex, i++);
            worldLayerIdDefault00 = file.getUint(recordIndex, i++);
            worldLayerIdDefault01 = file.getUint(recordIndex, i++);
            worldLayerIdDefault02 = file.getUint(recordIndex, i++);
            worldLayerIdDefault03 = file.getUint(recordIndex, i++);
        }
    };
}
