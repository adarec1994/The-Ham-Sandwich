#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct Creature2ActionSet
    {
        uint32_t ID;
        std::wstring description;
        uint32_t prerequisiteId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            description = file.getString(recordIndex, i++);
            prerequisiteId = file.getUint(recordIndex, i++);
        }
    };
}
