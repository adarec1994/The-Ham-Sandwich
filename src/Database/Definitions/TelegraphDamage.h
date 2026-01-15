#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TelegraphDamage
    {
        static constexpr const char* GetFileName() { return "TelegraphDamage"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t telegraphSubtypeEnum;
        uint32_t damageShapeEnum;
        float param00;
        float param01;
        float param02;
        float param03;
        float param04;
        float param05;
        uint32_t telegraphTimeStartMs;
        uint32_t telegraphTimeEndMs;
        uint32_t telegraphTimeRampInMs;
        uint32_t telegraphTimeRampOutMs;
        float xPositionOffset;
        float yPositionOffset;
        float zPositionOffset;
        float rotationDegrees;
        uint32_t telegraphDamageFlags;
        uint32_t targetTypeFlags;
        uint32_t phaseFlags;
        uint32_t prerequisiteIdCaster;
        uint32_t spellThresholdRestrictionFlags;
        uint32_t displayFlags;
        uint32_t opacityModifier;
        uint32_t displayGroup;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            telegraphSubtypeEnum = file.getUint(recordIndex, col++);
            damageShapeEnum = file.getUint(recordIndex, col++);
            param00 = file.getFloat(recordIndex, col++);
            param01 = file.getFloat(recordIndex, col++);
            param02 = file.getFloat(recordIndex, col++);
            param03 = file.getFloat(recordIndex, col++);
            param04 = file.getFloat(recordIndex, col++);
            param05 = file.getFloat(recordIndex, col++);
            telegraphTimeStartMs = file.getUint(recordIndex, col++);
            telegraphTimeEndMs = file.getUint(recordIndex, col++);
            telegraphTimeRampInMs = file.getUint(recordIndex, col++);
            telegraphTimeRampOutMs = file.getUint(recordIndex, col++);
            xPositionOffset = file.getFloat(recordIndex, col++);
            yPositionOffset = file.getFloat(recordIndex, col++);
            zPositionOffset = file.getFloat(recordIndex, col++);
            rotationDegrees = file.getFloat(recordIndex, col++);
            telegraphDamageFlags = file.getUint(recordIndex, col++);
            targetTypeFlags = file.getUint(recordIndex, col++);
            phaseFlags = file.getUint(recordIndex, col++);
            prerequisiteIdCaster = file.getUint(recordIndex, col++);
            spellThresholdRestrictionFlags = file.getUint(recordIndex, col++);
            displayFlags = file.getUint(recordIndex, col++);
            opacityModifier = file.getUint(recordIndex, col++);
            displayGroup = file.getUint(recordIndex, col++);
        }
    };
}