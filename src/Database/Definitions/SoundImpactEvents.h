#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct SoundImpactEvents
    {
        uint32_t ID;
        uint32_t origin;
        uint32_t target;
        uint32_t qualifier;
        uint32_t wEvent;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            origin = f.getUint(row, i++);
            target = f.getUint(row, i++);
            qualifier = f.getUint(row, i++);
            wEvent = f.getUint(row, i++);
        }
    };
}
