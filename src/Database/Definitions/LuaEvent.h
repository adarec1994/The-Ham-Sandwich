#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct LuaEvent
    {
        uint32_t ID;
        std::wstring eventName;
        std::wstring parameters;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            eventName = file.getString(recordIndex, i++);
            parameters = file.getString(recordIndex, i++);
        }
    };
}
