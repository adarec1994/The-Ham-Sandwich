#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct MatchingGameType
    {
        uint32_t ID;
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdDescription;
        uint32_t matchTypeEnum;
        uint32_t matchingGameTypeEnumFlags;
        uint32_t teamSize;
        uint32_t minLevel;
        uint32_t maxLevel;
        uint32_t preparationTimeMS;
        uint32_t matchTimeMS;
        uint32_t matchingRulesEnum;
        uint32_t matchingRulesData00;
        uint32_t matchingRulesData01;
        uint32_t targetItemLevel;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            localizedTextIdDescription = file.getUint(recordIndex, i++);
            matchTypeEnum = file.getUint(recordIndex, i++);
            matchingGameTypeEnumFlags = file.getUint(recordIndex, i++);
            teamSize = file.getUint(recordIndex, i++);
            minLevel = file.getUint(recordIndex, i++);
            maxLevel = file.getUint(recordIndex, i++);
            preparationTimeMS = file.getUint(recordIndex, i++);
            matchTimeMS = file.getUint(recordIndex, i++);
            matchingRulesEnum = file.getUint(recordIndex, i++);
            matchingRulesData00 = file.getUint(recordIndex, i++);
            matchingRulesData01 = file.getUint(recordIndex, i++);
            targetItemLevel = file.getUint(recordIndex, i++);
        }
    };
}
