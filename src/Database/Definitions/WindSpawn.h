#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct WindSpawn
    {
        static constexpr const char* GetFileName() { return "WindSpawn"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t intervalMin;
        uint32_t intervalMax;
        float directionMin;
        float directionMax;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            intervalMin = file.getUint(recordIndex, col++);
            intervalMax = file.getUint(recordIndex, col++);
            directionMin = file.getFloat(recordIndex, col++);
            directionMax = file.getFloat(recordIndex, col++);
        }
    };
}