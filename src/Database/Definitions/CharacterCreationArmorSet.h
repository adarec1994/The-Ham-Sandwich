#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct CharacterCreationArmorSet
    {
        uint32_t ID;
        uint32_t creationGearSetEnum;
        uint32_t classId;
        uint32_t itemDisplayId[12];

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            creationGearSetEnum = file.getUint(recordIndex, i++);
            classId = file.getUint(recordIndex, i++);
            for (int j = 0; j < 12; ++j)
                itemDisplayId[j] = file.getUint(recordIndex, i++);
        }
    };
}
