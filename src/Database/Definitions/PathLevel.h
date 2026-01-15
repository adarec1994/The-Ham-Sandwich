#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct PathLevel
    {
        uint32_t ID;
        uint32_t pathTypeEnum;
        uint32_t pathLevel;
        uint32_t pathXP;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            pathTypeEnum = file.getUint(recordIndex, i++);
            pathLevel = file.getUint(recordIndex, i++);
            pathXP = file.getUint(recordIndex, i++);
        }
    };
}
