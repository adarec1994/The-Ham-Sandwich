#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct HousingResource
    {
        uint32_t ID;
        uint32_t localizedTextIdName;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
        }
    };
}
