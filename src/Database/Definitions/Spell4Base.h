#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Spell4Base
    {
        uint32_t ID;
        uint32_t localizedTextIdName;
        uint32_t spell4HitResultId;
        uint32_t spell4TargetMechanicId;
        uint32_t spell4TargetAngleId;
        uint32_t spell4PrerequisiteId;
        uint32_t spell4ValidTargetId;
        uint32_t targetGroupIdCastGroup;
        uint32_t creature2IdPositionalAoe;
        float parameterAEAngle;
        float parameterAEMaxAngle;
        float parameterAEDistance;
        float parameterAEMaxDistance;
        uint32_t targetGroupIdAoeGroup;
        uint32_t spell4BaseIdPrerequisiteSpell;
        uint32_t worldZoneIdZoneRequired;
        uint32_t spell4SpellTypesIdSpellType;
        std::wstring icon;
        uint32_t castMethod;
        uint32_t school;
        uint32_t spellClass;
        uint32_t weaponSlot;
        uint32_t castBarType;
        float mechanicAggressionMagnitude;
        float mechanicDominationMagnitude;
        uint32_t modelSequencePriorityCaster;
        uint32_t modelSequencePriorityTarget;
        uint32_t classIdPlayer;
        uint32_t clientSideInteractionId;
        uint32_t targetingFlags;
        uint32_t telegraphFlagsEnum;
        uint32_t localizedTextIdLASTierPoint;
        float lasTierPointTooltipData00;
        float lasTierPointTooltipData01;
        float lasTierPointTooltipData02;
        float lasTierPointTooltipData03;
        float lasTierPointTooltipData04;
        float lasTierPointTooltipData05;
        float lasTierPointTooltipData06;
        float lasTierPointTooltipData07;
        float lasTierPointTooltipData08;
        float lasTierPointTooltipData09;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            localizedTextIdName = f.getUint(row, i++);
            spell4HitResultId = f.getUint(row, i++);
            spell4TargetMechanicId = f.getUint(row, i++);
            spell4TargetAngleId = f.getUint(row, i++);
            spell4PrerequisiteId = f.getUint(row, i++);
            spell4ValidTargetId = f.getUint(row, i++);
            targetGroupIdCastGroup = f.getUint(row, i++);
            creature2IdPositionalAoe = f.getUint(row, i++);
            parameterAEAngle = f.getFloat(row, i++);
            parameterAEMaxAngle = f.getFloat(row, i++);
            parameterAEDistance = f.getFloat(row, i++);
            parameterAEMaxDistance = f.getFloat(row, i++);
            targetGroupIdAoeGroup = f.getUint(row, i++);
            spell4BaseIdPrerequisiteSpell = f.getUint(row, i++);
            worldZoneIdZoneRequired = f.getUint(row, i++);
            spell4SpellTypesIdSpellType = f.getUint(row, i++);
            icon = f.getString(row, i++);
            castMethod = f.getUint(row, i++);
            school = f.getUint(row, i++);
            spellClass = f.getUint(row, i++);
            weaponSlot = f.getUint(row, i++);
            castBarType = f.getUint(row, i++);
            mechanicAggressionMagnitude = f.getFloat(row, i++);
            mechanicDominationMagnitude = f.getFloat(row, i++);
            modelSequencePriorityCaster = f.getUint(row, i++);
            modelSequencePriorityTarget = f.getUint(row, i++);
            classIdPlayer = f.getUint(row, i++);
            clientSideInteractionId = f.getUint(row, i++);
            targetingFlags = f.getUint(row, i++);
            telegraphFlagsEnum = f.getUint(row, i++);
            localizedTextIdLASTierPoint = f.getUint(row, i++);
            lasTierPointTooltipData00 = f.getFloat(row, i++);
            lasTierPointTooltipData01 = f.getFloat(row, i++);
            lasTierPointTooltipData02 = f.getFloat(row, i++);
            lasTierPointTooltipData03 = f.getFloat(row, i++);
            lasTierPointTooltipData04 = f.getFloat(row, i++);
            lasTierPointTooltipData05 = f.getFloat(row, i++);
            lasTierPointTooltipData06 = f.getFloat(row, i++);
            lasTierPointTooltipData07 = f.getFloat(row, i++);
            lasTierPointTooltipData08 = f.getFloat(row, i++);
            lasTierPointTooltipData09 = f.getFloat(row, i++);
        }
    };
}
