#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct RandomPlayerName
    {
        uint32_t ID;
        std::wstring nameFragment;
        uint32_t nameFragmentTypeEnum;
        uint32_t raceId;
        uint32_t gender;
        uint32_t faction2Id;
        uint32_t languageFlags;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            nameFragment = f.getString(row, i++);
            nameFragmentTypeEnum = f.getUint(row, i++);
            raceId = f.getUint(row, i++);
            gender = f.getUint(row, i++);
            faction2Id = f.getUint(row, i++);
            languageFlags = f.getUint(row, i++);
        }
    };
}
