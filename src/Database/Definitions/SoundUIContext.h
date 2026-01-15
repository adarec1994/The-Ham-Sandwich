#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct SoundUIContext
    {
        uint32_t ID;
        std::wstring luaVariableName;
        uint32_t soundEventId;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            luaVariableName = f.getString(row, i++);
            soundEventId = f.getUint(row, i++);
        }
    };
}
