#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct Class
    {
        uint32_t ID;
        std::wstring enumName;
        uint32_t localizedTextId;
        uint32_t localizedTextIdNameFemale;
        uint32_t mechanic;
        uint32_t spell4IdInnateAbilityActive[3];
        uint32_t spell4IdInnateAbilityPassive[3];
        uint32_t prerequisiteIdInnateAbility[3];
        uint32_t startingItemProficiencies;
        uint32_t spell4IdAttackPrimary[2];
        uint32_t spell4IdAttackUnarmed[2];
        uint32_t spell4IdResAbility;
        uint32_t localizedTextIdDescription;
        uint32_t spell4GroupId;
        uint32_t classIdForClassApModifier;
        uint32_t vitalEnumResource[8];
        uint32_t localizedTextIdResourceAlert[8];
        uint32_t attributeMilestoneGroupId[6];
        uint32_t classSecondaryStatBonusId[6];
        uint32_t attributeMiniMilestoneGroupId[6];
        uint32_t attributeMilestoneMaxTiers[6];

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            enumName = file.getString(recordIndex, i++);
            localizedTextId = file.getUint(recordIndex, i++);
            localizedTextIdNameFemale = file.getUint(recordIndex, i++);
            mechanic = file.getUint(recordIndex, i++);
            for (int j = 0; j < 3; ++j)
                spell4IdInnateAbilityActive[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 3; ++j)
                spell4IdInnateAbilityPassive[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 3; ++j)
                prerequisiteIdInnateAbility[j] = file.getUint(recordIndex, i++);
            startingItemProficiencies = file.getUint(recordIndex, i++);
            for (int j = 0; j < 2; ++j)
                spell4IdAttackPrimary[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 2; ++j)
                spell4IdAttackUnarmed[j] = file.getUint(recordIndex, i++);
            spell4IdResAbility = file.getUint(recordIndex, i++);
            localizedTextIdDescription = file.getUint(recordIndex, i++);
            spell4GroupId = file.getUint(recordIndex, i++);
            classIdForClassApModifier = file.getUint(recordIndex, i++);
            for (int j = 0; j < 8; ++j)
                vitalEnumResource[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 8; ++j)
                localizedTextIdResourceAlert[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 6; ++j)
                attributeMilestoneGroupId[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 6; ++j)
                classSecondaryStatBonusId[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 6; ++j)
                attributeMiniMilestoneGroupId[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 6; ++j)
                attributeMilestoneMaxTiers[j] = file.getUint(recordIndex, i++);
        }
    };
}
