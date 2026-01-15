#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct UnitRace
    {
        static constexpr const char* GetFileName() { return "UnitRace"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t soundImpactDescriptionIdOrigin;
        uint32_t soundImpactDescriptionIdTarget;
        uint32_t unitVisualTypeId;
        uint32_t soundEventIdAggro;
        uint32_t soundEventIdAware;
        uint32_t soundSwitchIdModel;
        uint32_t soundCombatLoopId;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            soundImpactDescriptionIdOrigin = file.getUint(recordIndex, col++);
            soundImpactDescriptionIdTarget = file.getUint(recordIndex, col++);
            unitVisualTypeId = file.getUint(recordIndex, col++);
            soundEventIdAggro = file.getUint(recordIndex, col++);
            soundEventIdAware = file.getUint(recordIndex, col++);
            soundSwitchIdModel = file.getUint(recordIndex, col++);
            soundCombatLoopId = file.getUint(recordIndex, col++);
        }
    };
}