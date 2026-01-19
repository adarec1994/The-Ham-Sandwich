#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct ItemSpecial
    {
        uint32_t ID;
        uint32_t prerequisiteIdGeneric00;
        uint32_t localizedTextIdName;
        uint32_t spell4IdOnEquip;
        uint32_t spell4IdOnActivate;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            prerequisiteIdGeneric00 = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            spell4IdOnEquip = file.getUint(recordIndex, i++);
            spell4IdOnActivate = file.getUint(recordIndex, i++);
        }
    };
}
