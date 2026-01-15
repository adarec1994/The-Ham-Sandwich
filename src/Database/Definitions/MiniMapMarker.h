#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct MiniMapMarker
    {
        uint32_t ID;
        std::wstring luaName;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            luaName = file.getString(recordIndex, i++);
        }
    };
}
