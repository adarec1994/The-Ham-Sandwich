#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct InstancePortal
    {
        uint32_t ID;
        uint32_t localizedTextIdName;
        uint32_t minLevel;
        uint32_t maxLevel;
        uint32_t expectedCompletionTime;
        uint32_t instancePortalTypeEnum;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            minLevel = file.getUint(recordIndex, i++);
            maxLevel = file.getUint(recordIndex, i++);
            expectedCompletionTime = file.getUint(recordIndex, i++);
            instancePortalTypeEnum = file.getUint(recordIndex, i++);
        }
    };
}
