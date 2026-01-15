#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Spell4ClientMissile
    {
        uint32_t ID;
        uint32_t missileType;
        uint32_t castStage;
        uint32_t originUnitEnum;
        uint32_t targetFlags;
        std::wstring modelPath;
        std::wstring fxPath;
        std::wstring beamPath;
        uint32_t beamSource;
        uint32_t beamTarget;
        uint32_t itemSlot;
        uint32_t costumeSide;
        uint32_t modelAttachmentIdCaster;
        uint32_t modelAttachmentIdTarget;
        uint32_t clientDelay;
        uint32_t modelEventIdDelayedBy;
        uint32_t flags;
        uint32_t duration;
        uint32_t frequency;
        uint32_t speedMps;
        float accMpss;
        uint32_t revolverNestedMissileInitDelay;
        uint32_t revolverNestedMissileSubDelay;
        uint32_t spell4ClientMissileIdNested;
        std::wstring revolverMissileImpactAssetPath;
        uint32_t missileRevolverTrackId;
        std::wstring birthAnchorPath;
        std::wstring deathAnchorPath;
        std::wstring trajAnchorPath;
        float birthDuration;
        float birthAnchorAngleMin;
        float birthAnchorAngleMax;
        float deathAnchorAngleMin;
        float deathAnchorAngleMax;
        uint32_t deathAnchorSpace;
        uint32_t itemSlotIdObj;
        uint32_t objCostumeSide;
        float trajPoseFullBlendDistance;
        float trajAnchorPlaySpeed;
        float parabolaHeightScale;
        float rotateX;
        float rotateY;
        float rotateZ;
        float scale;
        float endScale;
        uint32_t phaseFlags;
        uint32_t telegraphDamageIdAttach;
        uint32_t soundEventIdBirth;
        uint32_t soundEventIdLoopStart;
        uint32_t soundEventIdLoopStop;
        uint32_t soundEventIdDeath;
        uint32_t beamDiffuseColor;
        uint32_t missileDiffuseColor;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            missileType = f.getUint(row, i++);
            castStage = f.getUint(row, i++);
            originUnitEnum = f.getUint(row, i++);
            targetFlags = f.getUint(row, i++);
            modelPath = f.getString(row, i++);
            fxPath = f.getString(row, i++);
            beamPath = f.getString(row, i++);
            beamSource = f.getUint(row, i++);
            beamTarget = f.getUint(row, i++);
            itemSlot = f.getUint(row, i++);
            costumeSide = f.getUint(row, i++);
            modelAttachmentIdCaster = f.getUint(row, i++);
            modelAttachmentIdTarget = f.getUint(row, i++);
            clientDelay = f.getUint(row, i++);
            modelEventIdDelayedBy = f.getUint(row, i++);
            flags = f.getUint(row, i++);
            duration = f.getUint(row, i++);
            frequency = f.getUint(row, i++);
            speedMps = f.getUint(row, i++);
            accMpss = f.getFloat(row, i++);
            revolverNestedMissileInitDelay = f.getUint(row, i++);
            revolverNestedMissileSubDelay = f.getUint(row, i++);
            spell4ClientMissileIdNested = f.getUint(row, i++);
            revolverMissileImpactAssetPath = f.getString(row, i++);
            missileRevolverTrackId = f.getUint(row, i++);
            birthAnchorPath = f.getString(row, i++);
            deathAnchorPath = f.getString(row, i++);
            trajAnchorPath = f.getString(row, i++);
            birthDuration = f.getFloat(row, i++);
            birthAnchorAngleMin = f.getFloat(row, i++);
            birthAnchorAngleMax = f.getFloat(row, i++);
            deathAnchorAngleMin = f.getFloat(row, i++);
            deathAnchorAngleMax = f.getFloat(row, i++);
            deathAnchorSpace = f.getUint(row, i++);
            itemSlotIdObj = f.getUint(row, i++);
            objCostumeSide = f.getUint(row, i++);
            trajPoseFullBlendDistance = f.getFloat(row, i++);
            trajAnchorPlaySpeed = f.getFloat(row, i++);
            parabolaHeightScale = f.getFloat(row, i++);
            rotateX = f.getFloat(row, i++);
            rotateY = f.getFloat(row, i++);
            rotateZ = f.getFloat(row, i++);
            scale = f.getFloat(row, i++);
            endScale = f.getFloat(row, i++);
            phaseFlags = f.getUint(row, i++);
            telegraphDamageIdAttach = f.getUint(row, i++);
            soundEventIdBirth = f.getUint(row, i++);
            soundEventIdLoopStart = f.getUint(row, i++);
            soundEventIdLoopStop = f.getUint(row, i++);
            soundEventIdDeath = f.getUint(row, i++);
            beamDiffuseColor = f.getUint(row, i++);
            missileDiffuseColor = f.getUint(row, i++);
        }
    };
}
