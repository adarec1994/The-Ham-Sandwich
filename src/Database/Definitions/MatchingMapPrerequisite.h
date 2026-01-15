#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct MatchingMapPrerequisite
    {
        uint32_t ID;
        uint32_t matchingGameMapId;
        uint32_t matchingEligibilityFlagEnum;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            matchingGameMapId = file.getUint(recordIndex, i++);
            matchingEligibilityFlagEnum = file.getUint(recordIndex, i++);
        }
    };
}
