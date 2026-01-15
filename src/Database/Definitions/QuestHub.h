#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct QuestHub
    {
        uint32_t ID;
        uint32_t worldLocation2Id;
        uint32_t localizedTextIdName;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            worldLocation2Id = f.getUint(row, i++);
            localizedTextIdName = f.getUint(row, i++);
        }
    };
}
