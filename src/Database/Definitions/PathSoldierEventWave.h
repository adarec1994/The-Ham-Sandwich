#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PathSoldierEventWave
    {
        uint32_t ID;
        uint32_t pathSoldierEventId;
        uint32_t waveIndex;
        uint32_t soundZoneKitId;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            pathSoldierEventId = f.getUint(row, i++);
            waveIndex = f.getUint(row, i++);
            soundZoneKitId = f.getUint(row, i++);
        }
    };
}
