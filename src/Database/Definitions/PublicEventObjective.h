#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PublicEventObjective
    {
        uint32_t ID;
        uint32_t publicEventId;
        uint32_t publicEventObjectiveFlags;
        uint32_t publicEventObjectiveTypeSpecificFlags;
        uint32_t worldLocation2Id;
        uint32_t publicEventTeamId;
        uint32_t localizedTextId;
        uint32_t localizedTextIdOtherTeam;
        uint32_t localizedTextIdShort;
        uint32_t localizedTextIdOtherTeamShort;
        uint32_t publicEventObjectiveTypeEnum;
        uint32_t count;
        uint32_t objectId;
        uint32_t failureTimeMs;
        uint32_t targetGroupIdRewardPane;
        uint32_t publicEventObjectiveCategoryEnum;
        uint32_t liveEventIdCounter;
        uint32_t publicEventObjectiveIdParent;
        uint32_t questDirectionId;
        uint32_t medalPointValue;
        uint32_t localizedTextIdParticipantAdd;
        uint32_t localizedTextIdStart;
        uint32_t displayOrder;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            publicEventId = f.getUint(row, i++);
            publicEventObjectiveFlags = f.getUint(row, i++);
            publicEventObjectiveTypeSpecificFlags = f.getUint(row, i++);
            worldLocation2Id = f.getUint(row, i++);
            publicEventTeamId = f.getUint(row, i++);
            localizedTextId = f.getUint(row, i++);
            localizedTextIdOtherTeam = f.getUint(row, i++);
            localizedTextIdShort = f.getUint(row, i++);
            localizedTextIdOtherTeamShort = f.getUint(row, i++);
            publicEventObjectiveTypeEnum = f.getUint(row, i++);
            count = f.getUint(row, i++);
            objectId = f.getUint(row, i++);
            failureTimeMs = f.getUint(row, i++);
            targetGroupIdRewardPane = f.getUint(row, i++);
            publicEventObjectiveCategoryEnum = f.getUint(row, i++);
            liveEventIdCounter = f.getUint(row, i++);
            publicEventObjectiveIdParent = f.getUint(row, i++);
            questDirectionId = f.getUint(row, i++);
            medalPointValue = f.getUint(row, i++);
            localizedTextIdParticipantAdd = f.getUint(row, i++);
            localizedTextIdStart = f.getUint(row, i++);
            displayOrder = f.getUint(row, i++);
        }
    };
}
