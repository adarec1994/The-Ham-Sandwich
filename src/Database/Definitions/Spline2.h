#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct Spline2
    {
        static constexpr const char* GetFileName() { return "Spline2"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t worldId;
        uint32_t splineType;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            worldId = file.getUint(recordIndex, col++);
            splineType = file.getUint(recordIndex, col++);
        }
    };
}