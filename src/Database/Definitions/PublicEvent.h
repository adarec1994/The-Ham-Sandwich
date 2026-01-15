#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PublicEvent
    {
        uint32_t ID;
        uint32_t worldId;
        uint32_t worldZoneId;
        uint32_t localizedTextIdName;
        uint32_t failureTimeMs;
        uint32_t worldLocation2Id;
        uint32_t publicEventTypeEnum;
        uint32_t publicEventIdParent;
        uint32_t minPlayerLevel;
        uint32_t liveEventIdLifetime;
        uint32_t publicEventFlags;
        uint32_t localizedTextIdEnd;
        uint32_t rewardRotationContentId;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            worldId = f.getUint(row, i++);
            worldZoneId = f.getUint(row, i++);
            localizedTextIdName = f.getUint(row, i++);
            failureTimeMs = f.getUint(row, i++);
            worldLocation2Id = f.getUint(row, i++);
            publicEventTypeEnum = f.getUint(row, i++);
            publicEventIdParent = f.getUint(row, i++);
            minPlayerLevel = f.getUint(row, i++);
            liveEventIdLifetime = f.getUint(row, i++);
            publicEventFlags = f.getUint(row, i++);
            localizedTextIdEnd = f.getUint(row, i++);
            rewardRotationContentId = f.getUint(row, i++);
        }
    };
}
