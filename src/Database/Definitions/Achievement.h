#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct Achievement
    {
        uint32_t ID;
        uint32_t achievementTypeId;
        uint32_t achievementCategoryId;
        uint32_t flags;
        uint32_t worldZoneId;
        uint32_t localizedTextIdTitle;
        uint32_t localizedTextIdDesc;
        uint32_t localizedTextIdProgress;
        float percCompletionToShow;
        uint32_t objectId;
        uint32_t objectIdAlt;
        uint32_t value;
        uint32_t characterTitleId;
        uint32_t prerequisiteId;
        uint32_t prerequisiteIdServer;
        uint32_t prerequisiteIdObjective;
        uint32_t prerequisiteIdObjectiveAlt;
        uint32_t achievementIdParentTier;
        uint32_t orderIndex;
        uint32_t achievementGroupId;
        uint32_t achievementSubGroupId;
        uint32_t achievementPointEnum;
        std::wstring steamAchievementName;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            achievementTypeId = file.getUint(recordIndex, i++);
            achievementCategoryId = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            worldZoneId = file.getUint(recordIndex, i++);
            localizedTextIdTitle = file.getUint(recordIndex, i++);
            localizedTextIdDesc = file.getUint(recordIndex, i++);
            localizedTextIdProgress = file.getUint(recordIndex, i++);
            percCompletionToShow = file.getFloat(recordIndex, i++);
            objectId = file.getUint(recordIndex, i++);
            objectIdAlt = file.getUint(recordIndex, i++);
            value = file.getUint(recordIndex, i++);
            characterTitleId = file.getUint(recordIndex, i++);
            prerequisiteId = file.getUint(recordIndex, i++);
            prerequisiteIdServer = file.getUint(recordIndex, i++);
            prerequisiteIdObjective = file.getUint(recordIndex, i++);
            prerequisiteIdObjectiveAlt = file.getUint(recordIndex, i++);
            achievementIdParentTier = file.getUint(recordIndex, i++);
            orderIndex = file.getUint(recordIndex, i++);
            achievementGroupId = file.getUint(recordIndex, i++);
            achievementSubGroupId = file.getUint(recordIndex, i++);
            achievementPointEnum = file.getUint(recordIndex, i++);
            steamAchievementName = file.getString(recordIndex, i++);
        }
    };
}
