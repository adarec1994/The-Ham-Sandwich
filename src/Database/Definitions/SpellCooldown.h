#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct SpellCoolDown
    {
        static constexpr const char* GetFileName() { return "SpellCoolDown"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t cooldownTime;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            cooldownTime = file.getUint(recordIndex, col++);
        }
    };
}