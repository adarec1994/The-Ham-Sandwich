#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct VeteranTier
    {
        static constexpr const char* GetFileName() { return "VeteranTier"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t primeLevel;
        uint32_t veteranTierScalingType;
        uint32_t unitPropertyOverrideMenace;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            primeLevel = file.getUint(recordIndex, col++);
            veteranTierScalingType = file.getUint(recordIndex, col++);
            unitPropertyOverrideMenace = file.getUint(recordIndex, col++);
        }
    };
}