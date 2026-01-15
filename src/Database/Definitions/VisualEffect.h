#pragma once
#include <cstdint>
#include <string>

namespace Database::Definitions
{
    struct VisualEffect
    {
        static constexpr const char* GetFileName() { return "VisualEffect"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t visualType;
        uint32_t startDelay;
        uint32_t duration;
        uint32_t modelItemSlot;
        uint32_t modelItemCostumeSide;
        std::wstring modelAssetPath;
        uint32_t modelAttachmentId;
        uint32_t modelSequencePriority;
        uint32_t modelSequenceIdTarget00;
        uint32_t modelSequenceIdTarget01;
        uint32_t modelSequenceIdTarget02;
        float modelScale;
        float modelRotationX;
        float modelRotationY;
        float modelRotationZ;
        float data00;
        float data01;
        float data02;
        float data03;
        float data04;
        uint32_t flags;
        uint32_t soundEventId00;
        uint32_t soundEventId01;
        uint32_t soundEventId02;
        uint32_t soundEventId03;
        uint32_t soundEventId04;
        uint32_t soundEventId05;
        uint32_t soundEventOffset00;
        uint32_t soundEventOffset01;
        uint32_t soundEventOffset02;
        uint32_t soundEventOffset03;
        uint32_t soundEventOffset04;
        uint32_t soundEventOffset05;
        uint32_t soundEventIdStop;
        uint32_t soundZoneKitId;
        uint32_t prerequisiteId;
        uint32_t particleDiffuseColor;
    };
}