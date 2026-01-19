#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct Creature2
    {
        uint32_t ID;
        uint32_t CreationTypeEnum;
        std::wstring description;
        uint32_t localizedTextIdName;
        uint32_t creature2AoiSizeEnum;
        uint32_t unitRaceId;
        uint32_t creature2DifficultyId;
        uint32_t creature2ArcheTypeId;
        uint32_t creature2TierId;
        uint32_t creature2ModelInfoId;
        uint32_t creature2DisplayGroupId;
        uint32_t creature2OutfitGroupId;
        uint32_t prerequisiteIdVisibility;
        float modelScale;
        uint32_t spell4IdActivate[4];
        uint32_t prerequisiteIdActivateSpell[4];
        uint32_t activateSpellCastTime;
        float activateSpellMinRange;
        float activateSpellMaxRange;
        uint32_t localizedTextIdActivateSpellText;
        uint32_t spell4VisualGroupIdActivateSpell;
        uint32_t trainerClassIdMask;
        uint32_t tradeSkillIdTrainer;
        uint32_t tradeSkillIdStation;
        uint32_t questIdGiven[25];
        uint32_t questIdReceive[25];
        uint32_t questAnimStateId;
        uint32_t prerequisiteIdAnimState;
        uint32_t questAnimObjectiveIndex;
        uint32_t flags;
        uint32_t uiFlags;
        uint32_t activationFlags;
        float aimYawConstraint;
        float aimPitchUpConstraint;
        float aimPitchDownConstraint;
        uint32_t item2IdMTXKey[2];
        uint32_t creature2FamilyId;
        uint32_t creature2TractId;
        uint32_t bindPointId;
        uint32_t resourceConversionGroupId;
        uint32_t taxiNodeId;
        uint32_t pathScientistExperimentationId;
        uint32_t datacubeId;
        uint32_t datacubeVolumeId;
        uint32_t factionId;
        uint32_t minLevel;
        uint32_t maxLevel;
        uint32_t rescanCooldownTypeEnum;
        uint32_t rescanCooldown;
        uint32_t creature2AffiliationId;
        uint32_t itemIdDisplayItemRight;
        uint32_t soundEventIdAggro;
        uint32_t soundEventIdAware;
        uint32_t soundImpactDescriptionIdOrigin;
        uint32_t soundImpactDescriptionIdTarget;
        uint32_t soundSwitchIdModel;
        uint32_t soundCombatLoopId;
        uint32_t randomTextLineIdGoodbye[10];
        uint32_t randomTextLineIdHello[10];
        uint32_t randomTextLineIdIntro;
        uint32_t localizedTextIdDefaultGreeting;
        uint32_t randomTextLineIdReturn[10];
        uint32_t localizedTextIdReturnGreeting;
        uint32_t randomTextLineIdCompleted;
        uint32_t localizedTextIdCompletedGreeting;
        uint32_t unitVoiceTypeId;
        uint32_t gossipSetId;
        uint32_t unitVisualTypeId;
        uint32_t spell4VisualGroupIdAttached;
        uint32_t genericStringGroupsIdInteractContext;
        uint32_t creature2ActionSetId;
        uint32_t creature2ActionTextId;
        uint32_t pathMissionIdSoldier;
        uint32_t instancePortalId;
        uint32_t modelSequenceIdAnimationPriority[5];
        uint32_t prerequisiteIdPriority[5];
        float donutDrawDistance;
        uint32_t archiveArticleIdInteractUnlock;
        uint32_t tradeskillHarvestingInfoId;
        uint32_t ccStateImmunitiesFlags;
        uint32_t creature2ResistId;
        uint32_t unitVehicleId;
        uint32_t creature2DisplayInfoIdPortraitOverride;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            CreationTypeEnum = file.getUint(recordIndex, i++);
            description = file.getString(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            creature2AoiSizeEnum = file.getUint(recordIndex, i++);
            unitRaceId = file.getUint(recordIndex, i++);
            creature2DifficultyId = file.getUint(recordIndex, i++);
            creature2ArcheTypeId = file.getUint(recordIndex, i++);
            creature2TierId = file.getUint(recordIndex, i++);
            creature2ModelInfoId = file.getUint(recordIndex, i++);
            creature2DisplayGroupId = file.getUint(recordIndex, i++);
            creature2OutfitGroupId = file.getUint(recordIndex, i++);
            prerequisiteIdVisibility = file.getUint(recordIndex, i++);
            modelScale = file.getFloat(recordIndex, i++);
            for (int j = 0; j < 4; ++j)
                spell4IdActivate[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 4; ++j)
                prerequisiteIdActivateSpell[j] = file.getUint(recordIndex, i++);
            activateSpellCastTime = file.getUint(recordIndex, i++);
            activateSpellMinRange = file.getFloat(recordIndex, i++);
            activateSpellMaxRange = file.getFloat(recordIndex, i++);
            localizedTextIdActivateSpellText = file.getUint(recordIndex, i++);
            spell4VisualGroupIdActivateSpell = file.getUint(recordIndex, i++);
            trainerClassIdMask = file.getUint(recordIndex, i++);
            tradeSkillIdTrainer = file.getUint(recordIndex, i++);
            tradeSkillIdStation = file.getUint(recordIndex, i++);
            for (int j = 0; j < 25; ++j)
                questIdGiven[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 25; ++j)
                questIdReceive[j] = file.getUint(recordIndex, i++);
            questAnimStateId = file.getUint(recordIndex, i++);
            prerequisiteIdAnimState = file.getUint(recordIndex, i++);
            questAnimObjectiveIndex = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            uiFlags = file.getUint(recordIndex, i++);
            activationFlags = file.getUint(recordIndex, i++);
            aimYawConstraint = file.getFloat(recordIndex, i++);
            aimPitchUpConstraint = file.getFloat(recordIndex, i++);
            aimPitchDownConstraint = file.getFloat(recordIndex, i++);
            for (int j = 0; j < 2; ++j)
                item2IdMTXKey[j] = file.getUint(recordIndex, i++);
            creature2FamilyId = file.getUint(recordIndex, i++);
            creature2TractId = file.getUint(recordIndex, i++);
            bindPointId = file.getUint(recordIndex, i++);
            resourceConversionGroupId = file.getUint(recordIndex, i++);
            taxiNodeId = file.getUint(recordIndex, i++);
            pathScientistExperimentationId = file.getUint(recordIndex, i++);
            datacubeId = file.getUint(recordIndex, i++);
            datacubeVolumeId = file.getUint(recordIndex, i++);
            factionId = file.getUint(recordIndex, i++);
            minLevel = file.getUint(recordIndex, i++);
            maxLevel = file.getUint(recordIndex, i++);
            rescanCooldownTypeEnum = file.getUint(recordIndex, i++);
            rescanCooldown = file.getUint(recordIndex, i++);
            creature2AffiliationId = file.getUint(recordIndex, i++);
            itemIdDisplayItemRight = file.getUint(recordIndex, i++);
            soundEventIdAggro = file.getUint(recordIndex, i++);
            soundEventIdAware = file.getUint(recordIndex, i++);
            soundImpactDescriptionIdOrigin = file.getUint(recordIndex, i++);
            soundImpactDescriptionIdTarget = file.getUint(recordIndex, i++);
            soundSwitchIdModel = file.getUint(recordIndex, i++);
            soundCombatLoopId = file.getUint(recordIndex, i++);
            for (int j = 0; j < 10; ++j)
                randomTextLineIdGoodbye[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 10; ++j)
                randomTextLineIdHello[j] = file.getUint(recordIndex, i++);
            randomTextLineIdIntro = file.getUint(recordIndex, i++);
            localizedTextIdDefaultGreeting = file.getUint(recordIndex, i++);
            for (int j = 0; j < 10; ++j)
                randomTextLineIdReturn[j] = file.getUint(recordIndex, i++);
            localizedTextIdReturnGreeting = file.getUint(recordIndex, i++);
            randomTextLineIdCompleted = file.getUint(recordIndex, i++);
            localizedTextIdCompletedGreeting = file.getUint(recordIndex, i++);
            unitVoiceTypeId = file.getUint(recordIndex, i++);
            gossipSetId = file.getUint(recordIndex, i++);
            unitVisualTypeId = file.getUint(recordIndex, i++);
            spell4VisualGroupIdAttached = file.getUint(recordIndex, i++);
            genericStringGroupsIdInteractContext = file.getUint(recordIndex, i++);
            creature2ActionSetId = file.getUint(recordIndex, i++);
            creature2ActionTextId = file.getUint(recordIndex, i++);
            pathMissionIdSoldier = file.getUint(recordIndex, i++);
            instancePortalId = file.getUint(recordIndex, i++);
            for (int j = 0; j < 5; ++j)
                modelSequenceIdAnimationPriority[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 5; ++j)
                prerequisiteIdPriority[j] = file.getUint(recordIndex, i++);
            donutDrawDistance = file.getFloat(recordIndex, i++);
            archiveArticleIdInteractUnlock = file.getUint(recordIndex, i++);
            tradeskillHarvestingInfoId = file.getUint(recordIndex, i++);
            ccStateImmunitiesFlags = file.getUint(recordIndex, i++);
            creature2ResistId = file.getUint(recordIndex, i++);
            unitVehicleId = file.getUint(recordIndex, i++);
            creature2DisplayInfoIdPortraitOverride = file.getUint(recordIndex, i++);
        }
    };
}
