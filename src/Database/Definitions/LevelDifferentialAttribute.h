#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct LevelDifferentialAttribute
    {
        uint32_t ID;
        uint32_t localizedTextIdDescription;
        uint32_t levelDifferentialValue;
        float questXpMultiplier;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdDescription = file.getUint(recordIndex, i++);
            levelDifferentialValue = file.getUint(recordIndex, i++);
            questXpMultiplier = file.getFloat(recordIndex, i++);
        }
    };
}
