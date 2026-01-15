#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct SpellPhase
    {
        static constexpr const char* GetFileName() { return "SpellPhase"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t phaseDelay;
        uint32_t phaseFlags;
        uint32_t orderIndex;
        uint32_t spell4IdOwner;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            phaseDelay = file.getUint(recordIndex, col++);
            phaseFlags = file.getUint(recordIndex, col++);
            orderIndex = file.getUint(recordIndex, col++);
            spell4IdOwner = file.getUint(recordIndex, col++);
        }
    };
}