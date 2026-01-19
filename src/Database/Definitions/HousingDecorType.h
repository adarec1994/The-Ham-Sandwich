#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct HousingDecorType
    {
        uint32_t ID;
        uint32_t localizedTextId;
        std::wstring luaString;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextId = file.getUint(recordIndex, i++);
            luaString = file.getString(recordIndex, i++);
        }
    };
}
