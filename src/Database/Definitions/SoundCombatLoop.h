#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct SoundCombatLoop
    {
        uint32_t ID;
        uint32_t soundEventIdStart;
        uint32_t soundEventIdStop;
        uint32_t soundParameterIdUnitsInCombat;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            soundEventIdStart = f.getUint(row, i++);
            soundEventIdStop = f.getUint(row, i++);
            soundParameterIdUnitsInCombat = f.getUint(row, i++);
        }
    };
}
