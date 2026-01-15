#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct PathExplorerNode
    {
        uint32_t ID;
        uint32_t pathExplorerAreaId;
        uint32_t worldLocation2Id;
        uint32_t spline2Id;
        uint32_t localizedTextIdSettlerButton;
        uint32_t questDirectionId;
        uint32_t visualEffectId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            pathExplorerAreaId = file.getUint(recordIndex, i++);
            worldLocation2Id = file.getUint(recordIndex, i++);
            spline2Id = file.getUint(recordIndex, i++);
            localizedTextIdSettlerButton = file.getUint(recordIndex, i++);
            questDirectionId = file.getUint(recordIndex, i++);
            visualEffectId = file.getUint(recordIndex, i++);
        }
    };
}
