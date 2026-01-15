#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct MatchingGameMap
    {
        uint32_t ID;
        uint32_t matchingGameMapEnumFlags;
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdDescription;
        uint32_t matchingGameTypeId;
        uint32_t worldId;
        uint32_t recommendedItemLevel;
        uint32_t achievementCategoryId;
        uint32_t prerequisiteId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            matchingGameMapEnumFlags = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            localizedTextIdDescription = file.getUint(recordIndex, i++);
            matchingGameTypeId = file.getUint(recordIndex, i++);
            worldId = file.getUint(recordIndex, i++);
            recommendedItemLevel = file.getUint(recordIndex, i++);
            achievementCategoryId = file.getUint(recordIndex, i++);
            prerequisiteId = file.getUint(recordIndex, i++);
        }
    };
}
