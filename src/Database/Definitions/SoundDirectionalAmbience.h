#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct SoundDirectionalAmbience
    {
        uint32_t ID;
        uint32_t soundEventIdOutsideStart;
        uint32_t soundEventIdOutsideStop;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            soundEventIdOutsideStart = f.getUint(row, i++);
            soundEventIdOutsideStop = f.getUint(row, i++);
        }
    };
}
