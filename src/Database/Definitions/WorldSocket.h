#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct WorldSocket
    {
        static constexpr const char* GetFileName() { return "WorldSocket"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t worldId;
        uint32_t bounds0;
        uint32_t bounds1;
        uint32_t bounds2;
        uint32_t bounds3;
        uint32_t averageHeight;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            worldId = file.getUint(recordIndex, col++);
            bounds0 = file.getUint(recordIndex, col++);
            bounds1 = file.getUint(recordIndex, col++);
            bounds2 = file.getUint(recordIndex, col++);
            bounds3 = file.getUint(recordIndex, col++);
            averageHeight = file.getUint(recordIndex, col++);
        }
    };
}