#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct SoundSwitch
    {
        uint32_t ID;
        uint32_t nameHash;
        uint32_t groupHash;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            nameHash = f.getUint(row, i++);
            groupHash = f.getUint(row, i++);
        }
    };
}
