#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct CharacterCustomizationSelection
    {
        uint32_t ID;
        uint32_t characterCustomizationLabelId;
        uint32_t value;
        uint64_t cost;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            characterCustomizationLabelId = file.getUint(recordIndex, i++);
            value = file.getUint(recordIndex, i++);
            cost = static_cast<uint64_t>(file.getInt64(recordIndex, i++));
        }
    };
}
