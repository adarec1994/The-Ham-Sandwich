#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct CharacterCustomization
    {
        uint32_t ID;
        uint32_t raceId;
        uint32_t gender;
        uint32_t itemSlotId;
        uint32_t itemDisplayId;
        uint32_t flags;
        uint32_t characterCustomizationLabelId[2];
        uint32_t value[2];

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            raceId = file.getUint(recordIndex, i++);
            gender = file.getUint(recordIndex, i++);
            itemSlotId = file.getUint(recordIndex, i++);
            itemDisplayId = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            for (int j = 0; j < 2; ++j)
                characterCustomizationLabelId[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 2; ++j)
                value[j] = file.getUint(recordIndex, i++);
        }
    };
}
