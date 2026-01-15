#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct Spell4ServiceTokenCost
    {
        static constexpr const char* GetFileName() { return "Spell4ServiceTokenCost"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t spell4Id;
        uint32_t serviceTokenCost;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            spell4Id = file.getUint(recordIndex, col++);
            serviceTokenCost = file.getUint(recordIndex, col++);
        }
    };
}