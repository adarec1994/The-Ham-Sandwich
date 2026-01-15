#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct PathMission
    {
        uint32_t ID;
        uint32_t creature2IdUnlock;
        uint32_t pathTypeEnum;
        uint32_t pathMissionTypeEnum;
        uint32_t pathMissionDisplayTypeEnum;
        uint32_t objectId;
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdSummary;
        uint32_t pathEpisodeId;
        uint32_t worldLocation2Id00;
        uint32_t worldLocation2Id01;
        uint32_t worldLocation2Id02;
        uint32_t worldLocation2Id03;
        uint32_t pathMissionFlags;
        uint32_t pathMissionFactionEnum;
        uint32_t prerequisiteId;
        uint32_t localizedTextIdCommunicator;
        uint32_t localizedTextIdUnlock;
        uint32_t localizedTextIdSoldierOrders;
        uint32_t creature2IdContactOverride;
        uint32_t questDirectionId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            creature2IdUnlock = file.getUint(recordIndex, i++);
            pathTypeEnum = file.getUint(recordIndex, i++);
            pathMissionTypeEnum = file.getUint(recordIndex, i++);
            pathMissionDisplayTypeEnum = file.getUint(recordIndex, i++);
            objectId = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            localizedTextIdSummary = file.getUint(recordIndex, i++);
            pathEpisodeId = file.getUint(recordIndex, i++);
            worldLocation2Id00 = file.getUint(recordIndex, i++);
            worldLocation2Id01 = file.getUint(recordIndex, i++);
            worldLocation2Id02 = file.getUint(recordIndex, i++);
            worldLocation2Id03 = file.getUint(recordIndex, i++);
            pathMissionFlags = file.getUint(recordIndex, i++);
            pathMissionFactionEnum = file.getUint(recordIndex, i++);
            prerequisiteId = file.getUint(recordIndex, i++);
            localizedTextIdCommunicator = file.getUint(recordIndex, i++);
            localizedTextIdUnlock = file.getUint(recordIndex, i++);
            localizedTextIdSoldierOrders = file.getUint(recordIndex, i++);
            creature2IdContactOverride = file.getUint(recordIndex, i++);
            questDirectionId = file.getUint(recordIndex, i++);
        }
    };
}
