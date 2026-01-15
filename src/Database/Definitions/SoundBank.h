#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct SoundBank
    {
        uint32_t ID;
        std::wstring name;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            name = f.getString(row, i++);
        }
    };
}
