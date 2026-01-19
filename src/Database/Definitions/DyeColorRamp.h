#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct DyeColorRamp
    {
        uint32_t ID;
        uint32_t flags;
        uint32_t localizedTextIdName;
        uint32_t rampIndex;
        float costMultiplier;
        uint32_t componentMapEnum;
        uint32_t prerequisiteId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            rampIndex = file.getUint(recordIndex, i++);
            costMultiplier = file.getFloat(recordIndex, i++);
            componentMapEnum = file.getUint(recordIndex, i++);
            prerequisiteId = file.getUint(recordIndex, i++);
        }
    };
}
