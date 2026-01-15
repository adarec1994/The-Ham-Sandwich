#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct Spell4Telegraph
    {
        static constexpr const char* GetFileName() { return "Spell4Telegraph"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t spell4Id;
        uint32_t telegraphDamageId;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            spell4Id = file.getUint(recordIndex, col++);
            telegraphDamageId = file.getUint(recordIndex, col++);
        }
    };
}