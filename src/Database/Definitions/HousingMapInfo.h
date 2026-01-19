#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct HousingMapInfo
    {
        uint32_t ID;
        uint32_t worldId;
        uint32_t privatePropertyCount;
        uint32_t publicPropertyCount;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            worldId = file.getUint(recordIndex, i++);
            privatePropertyCount = file.getUint(recordIndex, i++);
            publicPropertyCount = file.getUint(recordIndex, i++);
        }
    };
}
