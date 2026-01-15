#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct SoundContext
    {
        uint32_t ID;
        uint32_t eventId;
        uint32_t type;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            eventId = f.getUint(row, i++);
            type = f.getUint(row, i++);
        }
    };
}
