#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct Vital
    {
        static constexpr const char* GetFileName() { return "Vital"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t localizedTextIdDisplayText;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            localizedTextIdDisplayText = file.getUint(recordIndex, col++);
        }
    };
}