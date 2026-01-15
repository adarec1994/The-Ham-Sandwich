#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct WorldLocation2
    {
        static constexpr const char* GetFileName() { return "WorldLocation2"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        float radius;
        float maxVerticalDistance;
        float position0;
        float position1;
        float position2;
        float facing0;
        float facing1;
        float facing2;
        float facing3;
        uint32_t worldId;
        uint32_t worldZoneId;
        uint32_t phases;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            radius = file.getFloat(recordIndex, col++);
            maxVerticalDistance = file.getFloat(recordIndex, col++);
            position0 = file.getFloat(recordIndex, col++);
            position1 = file.getFloat(recordIndex, col++);
            position2 = file.getFloat(recordIndex, col++);
            facing0 = file.getFloat(recordIndex, col++);
            facing1 = file.getFloat(recordIndex, col++);
            facing2 = file.getFloat(recordIndex, col++);
            facing3 = file.getFloat(recordIndex, col++);
            worldId = file.getUint(recordIndex, col++);
            worldZoneId = file.getUint(recordIndex, col++);
            phases = file.getUint(recordIndex, col++);
        }
    };
}