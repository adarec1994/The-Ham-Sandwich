#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct Spline2Node
    {
        static constexpr const char* GetFileName() { return "Spline2Node"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t splineId;
        uint32_t ordinal;
        float position0;
        float position1;
        float position2;
        float facing0;
        float facing1;
        float facing2;
        float facing3;
        uint32_t eventId;
        float frameTime;
        float delay;
        float fovy;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            splineId = file.getUint(recordIndex, col++);
            ordinal = file.getUint(recordIndex, col++);
            position0 = file.getFloat(recordIndex, col++);
            position1 = file.getFloat(recordIndex, col++);
            position2 = file.getFloat(recordIndex, col++);
            facing0 = file.getFloat(recordIndex, col++);
            facing1 = file.getFloat(recordIndex, col++);
            facing2 = file.getFloat(recordIndex, col++);
            facing3 = file.getFloat(recordIndex, col++);
            eventId = file.getUint(recordIndex, col++);
            frameTime = file.getFloat(recordIndex, col++);
            delay = file.getFloat(recordIndex, col++);
            fovy = file.getFloat(recordIndex, col++);
        }
    };
}