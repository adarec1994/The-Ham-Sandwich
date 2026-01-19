#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct CityDirection
    {
        uint32_t ID;
        uint32_t cityDirectionTypeEnum;
        uint32_t localizedTextIdName;
        uint32_t worldZoneId;
        uint32_t worldLocation2Id[4];

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            cityDirectionTypeEnum = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            worldZoneId = file.getUint(recordIndex, i++);
            for (int j = 0; j < 4; ++j)
                worldLocation2Id[j] = file.getUint(recordIndex, i++);
        }
    };
}
