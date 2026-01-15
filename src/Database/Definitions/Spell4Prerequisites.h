#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct Spell4Prerequisites
    {
        static constexpr const char* GetFileName() { return "Spell4Prerequisites"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t flags;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            flags = file.getUint(recordIndex, col++);
        }
    };
}