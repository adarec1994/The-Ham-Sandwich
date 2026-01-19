#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct Episode
    {
        uint32_t ID;
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdBriefing;
        uint32_t localizedTextIdEndSummary;
        uint32_t flags;
        uint32_t worldZoneId;
        uint32_t percentToDisplay;
        uint32_t questHubIdExile;
        uint32_t questHubIdDominion;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            localizedTextIdBriefing = file.getUint(recordIndex, i++);
            localizedTextIdEndSummary = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            worldZoneId = file.getUint(recordIndex, i++);
            percentToDisplay = file.getUint(recordIndex, i++);
            questHubIdExile = file.getUint(recordIndex, i++);
            questHubIdDominion = file.getUint(recordIndex, i++);
        }
    };
}
