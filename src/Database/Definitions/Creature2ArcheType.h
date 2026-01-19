#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct Creature2ArcheType
    {
        uint32_t ID;
        std::wstring icon;
        float unitPropertyMultiplier[200];

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            icon = file.getString(recordIndex, i++);
            for (int j = 0; j < 200; ++j)
                unitPropertyMultiplier[j] = file.getFloat(recordIndex, i++);
        }
    };
}
