#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct AccountItemCooldownGroup
    {
        uint32_t ID;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
        }
    };
}
