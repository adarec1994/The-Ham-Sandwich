#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct CharacterCreationPreset
    {
        uint32_t ID;
        uint32_t raceId;
        uint32_t faction2Id;
        uint32_t gender;
        std::wstring stringPreset;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            raceId = file.getUint(recordIndex, i++);
            faction2Id = file.getUint(recordIndex, i++);
            gender = file.getUint(recordIndex, i++);
            stringPreset = file.getString(recordIndex, i++);
        }
    };
}
