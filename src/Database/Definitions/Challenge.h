#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Challenge
    {
        uint32_t ID;
        uint32_t challengeTypeEnum;
        uint32_t target;
        uint32_t challengeFlags;
        uint32_t worldZoneIdRestriction;
        uint32_t triggerVolume2IdRestriction;
        uint32_t worldZoneId;
        uint32_t worldLocation2IdIndicator;
        uint32_t worldLocation2IdStartLocation;
        uint32_t completionCount;
        uint32_t challengeTierId[3];
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdProgress;
        uint32_t localizedTextIdAreaRestriction;
        uint32_t localizedTextIdLocation;
        uint32_t virtualItemIdDisplay;
        uint32_t targetGroupIdRewardPane;
        uint32_t questDirectionIdActive;
        uint32_t questDirectionIdInactive;
        uint32_t rewardTrackId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            challengeTypeEnum = file.getUint(recordIndex, i++);
            target = file.getUint(recordIndex, i++);
            challengeFlags = file.getUint(recordIndex, i++);
            worldZoneIdRestriction = file.getUint(recordIndex, i++);
            triggerVolume2IdRestriction = file.getUint(recordIndex, i++);
            worldZoneId = file.getUint(recordIndex, i++);
            worldLocation2IdIndicator = file.getUint(recordIndex, i++);
            worldLocation2IdStartLocation = file.getUint(recordIndex, i++);
            completionCount = file.getUint(recordIndex, i++);
            for (int j = 0; j < 3; ++j)
                challengeTierId[j] = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            localizedTextIdProgress = file.getUint(recordIndex, i++);
            localizedTextIdAreaRestriction = file.getUint(recordIndex, i++);
            localizedTextIdLocation = file.getUint(recordIndex, i++);
            virtualItemIdDisplay = file.getUint(recordIndex, i++);
            targetGroupIdRewardPane = file.getUint(recordIndex, i++);
            questDirectionIdActive = file.getUint(recordIndex, i++);
            questDirectionIdInactive = file.getUint(recordIndex, i++);
            rewardTrackId = file.getUint(recordIndex, i++);
        }
    };
}
