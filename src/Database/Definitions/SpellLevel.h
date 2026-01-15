#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct SpellLevel
    {
        static constexpr const char* GetFileName() { return "SpellLevel"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t classId;
        uint32_t characterLevel;
        uint32_t prerequisiteId;
        uint32_t spell4Id;
        float costMultiplier;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            classId = file.getUint(recordIndex, col++);
            characterLevel = file.getUint(recordIndex, col++);
            prerequisiteId = file.getUint(recordIndex, col++);
            spell4Id = file.getUint(recordIndex, col++);
            costMultiplier = file.getFloat(recordIndex, col++);
        }
    };
}