#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct WaterSurfaceEffect
    {
        static constexpr const char* GetFileName() { return "WaterSurfaceEffect"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t emissionDelay;
        uint32_t worldWaterWakeIdStillWater0;
        uint32_t worldWaterWakeIdStillWater1;
        uint32_t visualEffectIdParticle0;
        uint32_t visualEffectIdParticle1;
        uint32_t particleFlags0;
        uint32_t particleFlags1;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            emissionDelay = file.getUint(recordIndex, col++);
            worldWaterWakeIdStillWater0 = file.getUint(recordIndex, col++);
            worldWaterWakeIdStillWater1 = file.getUint(recordIndex, col++);
            visualEffectIdParticle0 = file.getUint(recordIndex, col++);
            visualEffectIdParticle1 = file.getUint(recordIndex, col++);
            particleFlags0 = file.getUint(recordIndex, col++);
            particleFlags1 = file.getUint(recordIndex, col++);
        }
    };
}