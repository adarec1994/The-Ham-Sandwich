#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct CommunicatorMessages
    {
        uint32_t ID;
        uint32_t localizedTextIdMessage;
        uint32_t delay;
        uint32_t flags;
        uint32_t creatureId;
        uint32_t worldId;
        uint32_t worldZoneId;
        uint32_t minLevel;
        uint32_t maxLevel;
        uint32_t quest[3];
        uint32_t state[3];
        uint32_t factionId;
        uint32_t classId;
        uint32_t raceId;
        uint32_t factionIdReputation;
        uint32_t reputationMin;
        uint32_t reputationMax;
        uint32_t questIdDelivered;
        uint32_t prerequisiteId;
        uint32_t displayDuration;
        uint32_t communicatorMessagesIdNext;
        uint32_t communicatorPortraitPlacementEnum;
        uint32_t communicatorOverlayEnum;
        uint32_t communicatorBackgroundEnum;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdMessage = file.getUint(recordIndex, i++);
            delay = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            creatureId = file.getUint(recordIndex, i++);
            worldId = file.getUint(recordIndex, i++);
            worldZoneId = file.getUint(recordIndex, i++);
            minLevel = file.getUint(recordIndex, i++);
            maxLevel = file.getUint(recordIndex, i++);
            for (int j = 0; j < 3; ++j)
                quest[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 3; ++j)
                state[j] = file.getUint(recordIndex, i++);
            factionId = file.getUint(recordIndex, i++);
            classId = file.getUint(recordIndex, i++);
            raceId = file.getUint(recordIndex, i++);
            factionIdReputation = file.getUint(recordIndex, i++);
            reputationMin = file.getUint(recordIndex, i++);
            reputationMax = file.getUint(recordIndex, i++);
            questIdDelivered = file.getUint(recordIndex, i++);
            prerequisiteId = file.getUint(recordIndex, i++);
            displayDuration = file.getUint(recordIndex, i++);
            communicatorMessagesIdNext = file.getUint(recordIndex, i++);
            communicatorPortraitPlacementEnum = file.getUint(recordIndex, i++);
            communicatorOverlayEnum = file.getUint(recordIndex, i++);
            communicatorBackgroundEnum = file.getUint(recordIndex, i++);
        }
    };
}
