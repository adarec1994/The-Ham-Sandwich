#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct Spell4TierRequirements
    {
        static constexpr const char* GetFileName() { return "Spell4TierRequirements"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t tierIndex;
        uint32_t levelRequirement;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            tierIndex = file.getUint(recordIndex, col++);
            levelRequirement = file.getUint(recordIndex, col++);
        }
    };
}