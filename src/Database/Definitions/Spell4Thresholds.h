#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct Spell4Thresholds
    {
        static constexpr const char* GetFileName() { return "Spell4Thresholds"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t spell4IdParent;
        uint32_t spell4IdToCast;
        uint32_t orderIndex;
        uint32_t thresholdDuration;
        uint32_t vitalEnumCostType00;
        uint32_t vitalEnumCostType01;
        uint32_t vitalCostValue00;
        uint32_t vitalCostValue01;
        uint32_t localizedTextIdTooltip;
        std::wstring iconReplacement;
        uint32_t visualEffectId;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            spell4IdParent = file.getUint(recordIndex, col++);
            spell4IdToCast = file.getUint(recordIndex, col++);
            orderIndex = file.getUint(recordIndex, col++);
            thresholdDuration = file.getUint(recordIndex, col++);
            vitalEnumCostType00 = file.getUint(recordIndex, col++);
            vitalEnumCostType01 = file.getUint(recordIndex, col++);
            vitalCostValue00 = file.getUint(recordIndex, col++);
            vitalCostValue01 = file.getUint(recordIndex, col++);
            localizedTextIdTooltip = file.getUint(recordIndex, col++);
            iconReplacement = file.getString(recordIndex, col++);
            visualEffectId = file.getUint(recordIndex, col++);
        }
    };
}