#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PathSoldierEvent
    {
        uint32_t ID;
        uint32_t pathSoldierEventType;
        uint32_t maxTimeBetweenWaves;
        uint32_t maxEventTime;
        uint32_t towerDefenseAllowance;
        uint32_t towerDefenseBuildTimeMS;
        uint32_t initialSpawnTime;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            pathSoldierEventType = f.getUint(row, i++);
            maxTimeBetweenWaves = f.getUint(row, i++);
            maxEventTime = f.getUint(row, i++);
            towerDefenseAllowance = f.getUint(row, i++);
            towerDefenseBuildTimeMS = f.getUint(row, i++);
            initialSpawnTime = f.getUint(row, i++);
        }
    };
}
