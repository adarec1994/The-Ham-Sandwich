#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct SoundMusicSet
    {
        uint32_t ID;
        uint32_t soundEventIdStart;
        uint32_t soundEventIdStop;
        float restartDelayMin;
        float restartDelayMax;
        uint32_t flags;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            soundEventIdStart = f.getUint(row, i++);
            soundEventIdStop = f.getUint(row, i++);
            restartDelayMin = f.getFloat(row, i++);
            restartDelayMax = f.getFloat(row, i++);
            flags = f.getUint(row, i++);
        }
    };
}
