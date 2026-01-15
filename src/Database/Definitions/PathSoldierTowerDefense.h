#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PathSoldierTowerDefense
    {
        uint32_t ID;
        uint32_t pathSoldierEventId;
        uint32_t buildCost;
        uint32_t localizedTextIdName;
        uint32_t worldLocation2IdDisplay;
        uint32_t towerDefenseBuildType;
        uint32_t spell4Id;
        uint32_t soldierTowerDefenseFlags;
        uint32_t soldierTowerDefenseImprovementType;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            pathSoldierEventId = f.getUint(row, i++);
            buildCost = f.getUint(row, i++);
            localizedTextIdName = f.getUint(row, i++);
            worldLocation2IdDisplay = f.getUint(row, i++);
            towerDefenseBuildType = f.getUint(row, i++);
            spell4Id = f.getUint(row, i++);
            soldierTowerDefenseFlags = f.getUint(row, i++);
            soldierTowerDefenseImprovementType = f.getUint(row, i++);
        }
    };
}
