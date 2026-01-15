#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct Spell4ValidTargets
    {
        static constexpr const char* GetFileName() { return "Spell4ValidTargets"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t targetBitmask;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            targetBitmask = file.getUint(recordIndex, col++);
        }
    };
}