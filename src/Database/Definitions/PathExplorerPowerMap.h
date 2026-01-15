#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct PathExplorerPowerMap
    {
        uint32_t ID;
        uint32_t distanceThreshold;
        uint32_t collectQuantity;
        uint32_t victoryPauseMS;
        uint32_t worldLocation2IdVisual;
        uint32_t visualEffectIdInactive;
        uint32_t localizedTextIdInfo;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            distanceThreshold = file.getUint(recordIndex, i++);
            collectQuantity = file.getUint(recordIndex, i++);
            victoryPauseMS = file.getUint(recordIndex, i++);
            worldLocation2IdVisual = file.getUint(recordIndex, i++);
            visualEffectIdInactive = file.getUint(recordIndex, i++);
            localizedTextIdInfo = file.getUint(recordIndex, i++);
        }
    };
}
