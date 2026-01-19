#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct FinishingMoveDeathVisual
    {
        uint32_t ID;
        uint32_t priority;
        uint32_t damageTypeFlags;
        uint32_t creature2MinSize;
        uint32_t creature2MaxSize;
        uint32_t creatureMaterialEnum;
        uint32_t movementStateFlags;
        std::wstring deathModelAsset;
        uint32_t modelSequenceIdDeath;
        uint32_t visualEffectIdDeath00;
        uint32_t visualEffectIdDeath01;
        uint32_t visualEffectIdDeath02;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            priority = file.getUint(recordIndex, i++);
            damageTypeFlags = file.getUint(recordIndex, i++);
            creature2MinSize = file.getUint(recordIndex, i++);
            creature2MaxSize = file.getUint(recordIndex, i++);
            creatureMaterialEnum = file.getUint(recordIndex, i++);
            movementStateFlags = file.getUint(recordIndex, i++);
            deathModelAsset = file.getString(recordIndex, i++);
            modelSequenceIdDeath = file.getUint(recordIndex, i++);
            visualEffectIdDeath00 = file.getUint(recordIndex, i++);
            visualEffectIdDeath01 = file.getUint(recordIndex, i++);
            visualEffectIdDeath02 = file.getUint(recordIndex, i++);
        }
    };
}
