#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct Spell4Visual
    {
        static constexpr const char* GetFileName() { return "Spell4Visual"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t targetTypeFlags;
        uint32_t stageType;
        uint32_t stageFlags;
        uint32_t visualEffectId;
        uint32_t visualEffectIdSound;
        uint32_t modelEventIdDelay;
        uint32_t soundEventType00;
        uint32_t soundEventType01;
        uint32_t soundEventType02;
        uint32_t soundEventType03;
        uint32_t soundEventType04;
        uint32_t soundEventType05;
        uint32_t soundImpactDescriptionIdTarget00;
        uint32_t soundImpactDescriptionIdTarget01;
        uint32_t soundImpactDescriptionIdTarget02;
        uint32_t soundImpactDescriptionIdTarget03;
        uint32_t soundImpactDescriptionIdTarget04;
        uint32_t soundImpactDescriptionIdTarget05;
        uint32_t soundImpactDescriptionIdOrigin00;
        uint32_t soundImpactDescriptionIdOrigin01;
        uint32_t soundImpactDescriptionIdOrigin02;
        uint32_t soundImpactDescriptionIdOrigin03;
        uint32_t soundImpactDescriptionIdOrigin04;
        uint32_t soundImpactDescriptionIdOrigin05;
        uint32_t modelAttachmentIdCaster;
        uint32_t phaseFlags;
        float modelOffsetX;
        float modelOffsetY;
        float modelOffsetZ;
        float modelScale;
        uint32_t preDelayTimeMs;
        uint32_t telegraphDamageIdAttach;
        uint32_t prerequisiteId;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            targetTypeFlags = file.getUint(recordIndex, col++);
            stageType = file.getUint(recordIndex, col++);
            stageFlags = file.getUint(recordIndex, col++);
            visualEffectId = file.getUint(recordIndex, col++);
            visualEffectIdSound = file.getUint(recordIndex, col++);
            modelEventIdDelay = file.getUint(recordIndex, col++);
            soundEventType00 = file.getUint(recordIndex, col++);
            soundEventType01 = file.getUint(recordIndex, col++);
            soundEventType02 = file.getUint(recordIndex, col++);
            soundEventType03 = file.getUint(recordIndex, col++);
            soundEventType04 = file.getUint(recordIndex, col++);
            soundEventType05 = file.getUint(recordIndex, col++);
            soundImpactDescriptionIdTarget00 = file.getUint(recordIndex, col++);
            soundImpactDescriptionIdTarget01 = file.getUint(recordIndex, col++);
            soundImpactDescriptionIdTarget02 = file.getUint(recordIndex, col++);
            soundImpactDescriptionIdTarget03 = file.getUint(recordIndex, col++);
            soundImpactDescriptionIdTarget04 = file.getUint(recordIndex, col++);
            soundImpactDescriptionIdTarget05 = file.getUint(recordIndex, col++);
            soundImpactDescriptionIdOrigin00 = file.getUint(recordIndex, col++);
            soundImpactDescriptionIdOrigin01 = file.getUint(recordIndex, col++);
            soundImpactDescriptionIdOrigin02 = file.getUint(recordIndex, col++);
            soundImpactDescriptionIdOrigin03 = file.getUint(recordIndex, col++);
            soundImpactDescriptionIdOrigin04 = file.getUint(recordIndex, col++);
            soundImpactDescriptionIdOrigin05 = file.getUint(recordIndex, col++);
            modelAttachmentIdCaster = file.getUint(recordIndex, col++);
            phaseFlags = file.getUint(recordIndex, col++);
            modelOffsetX = file.getFloat(recordIndex, col++);
            modelOffsetY = file.getFloat(recordIndex, col++);
            modelOffsetZ = file.getFloat(recordIndex, col++);
            modelScale = file.getFloat(recordIndex, col++);
            preDelayTimeMs = file.getUint(recordIndex, col++);
            telegraphDamageIdAttach = file.getUint(recordIndex, col++);
            prerequisiteId = file.getUint(recordIndex, col++);
        }
    };
}