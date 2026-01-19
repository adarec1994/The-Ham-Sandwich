#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct ClientSideInteraction
    {
        uint32_t ID;
        uint32_t interactionType;
        uint32_t threshold;
        uint32_t duration;
        uint32_t incrementValue;
        uint32_t windowSize;
        uint32_t decay;
        uint32_t flags;
        uint32_t tapTime[2];
        uint32_t visualEffectId[4];
        uint32_t visualEffectIdTarget[5];
        uint32_t visualEffectIdCaster[5];
        uint32_t localizedTextIdContext;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            interactionType = file.getUint(recordIndex, i++);
            threshold = file.getUint(recordIndex, i++);
            duration = file.getUint(recordIndex, i++);
            incrementValue = file.getUint(recordIndex, i++);
            windowSize = file.getUint(recordIndex, i++);
            decay = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            for (int j = 0; j < 2; ++j)
                tapTime[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 4; ++j)
                visualEffectId[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 5; ++j)
                visualEffectIdTarget[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 5; ++j)
                visualEffectIdCaster[j] = file.getUint(recordIndex, i++);
            localizedTextIdContext = file.getUint(recordIndex, i++);
        }
    };
}
