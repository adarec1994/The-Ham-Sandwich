#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct PathEpisode
    {
        uint32_t ID;
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdSummary;
        uint32_t worldId;
        uint32_t worldZoneId;
        uint32_t pathTypeEnum;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            localizedTextIdSummary = file.getUint(recordIndex, i++);
            worldId = file.getUint(recordIndex, i++);
            worldZoneId = file.getUint(recordIndex, i++);
            pathTypeEnum = file.getUint(recordIndex, i++);
        }
    };
}
