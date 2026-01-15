#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct PathScientistScanBotProfile
    {
        uint32_t ID;
        uint32_t localizedTextId;
        uint32_t creature2Id;
        uint32_t scanTimeMS;
        uint32_t processingTimeMS;
        float playerRadius;
        float scanAOERange;
        float maxSeekDistance;
        float speedMultiplier;
        float thoroughnessMultiplier;
        float healthMultiplier;
        float healthRegenMultiplier;
        uint32_t minCooldownTimeMs;
        uint32_t maxCooldownTimeMs;
        float maxCooldownDistance;
        uint32_t pathScientistScanBotProfileFlags;
        uint32_t socketCount;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextId = file.getUint(recordIndex, i++);
            creature2Id = file.getUint(recordIndex, i++);
            scanTimeMS = file.getUint(recordIndex, i++);
            processingTimeMS = file.getUint(recordIndex, i++);
            playerRadius = file.getFloat(recordIndex, i++);
            scanAOERange = file.getFloat(recordIndex, i++);
            maxSeekDistance = file.getFloat(recordIndex, i++);
            speedMultiplier = file.getFloat(recordIndex, i++);
            thoroughnessMultiplier = file.getFloat(recordIndex, i++);
            healthMultiplier = file.getFloat(recordIndex, i++);
            healthRegenMultiplier = file.getFloat(recordIndex, i++);
            minCooldownTimeMs = file.getUint(recordIndex, i++);
            maxCooldownTimeMs = file.getUint(recordIndex, i++);
            maxCooldownDistance = file.getFloat(recordIndex, i++);
            pathScientistScanBotProfileFlags = file.getUint(recordIndex, i++);
            socketCount = file.getUint(recordIndex, i++);
        }
    };
}
