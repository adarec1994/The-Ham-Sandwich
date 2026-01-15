#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PathSettlerMayor
    {
        uint32_t ID;
        uint32_t localizedTextId00;
        uint32_t localizedTextId01;
        uint32_t localizedTextId02;
        uint32_t localizedTextId03;
        uint32_t localizedTextId04;
        uint32_t localizedTextId05;
        uint32_t localizedTextId06;
        uint32_t localizedTextId07;
        uint32_t worldLocation2Id00;
        uint32_t worldLocation2Id01;
        uint32_t worldLocation2Id02;
        uint32_t worldLocation2Id03;
        uint32_t worldLocation2Id04;
        uint32_t worldLocation2Id05;
        uint32_t worldLocation2Id06;
        uint32_t worldLocation2Id07;
        uint32_t questDirectionId00;
        uint32_t questDirectionId01;
        uint32_t questDirectionId02;
        uint32_t questDirectionId03;
        uint32_t questDirectionId04;
        uint32_t questDirectionId05;
        uint32_t questDirectionId06;
        uint32_t questDirectionId07;
        uint32_t characterTitleIdReward;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            localizedTextId00 = f.getUint(row, i++);
            localizedTextId01 = f.getUint(row, i++);
            localizedTextId02 = f.getUint(row, i++);
            localizedTextId03 = f.getUint(row, i++);
            localizedTextId04 = f.getUint(row, i++);
            localizedTextId05 = f.getUint(row, i++);
            localizedTextId06 = f.getUint(row, i++);
            localizedTextId07 = f.getUint(row, i++);
            worldLocation2Id00 = f.getUint(row, i++);
            worldLocation2Id01 = f.getUint(row, i++);
            worldLocation2Id02 = f.getUint(row, i++);
            worldLocation2Id03 = f.getUint(row, i++);
            worldLocation2Id04 = f.getUint(row, i++);
            worldLocation2Id05 = f.getUint(row, i++);
            worldLocation2Id06 = f.getUint(row, i++);
            worldLocation2Id07 = f.getUint(row, i++);
            questDirectionId00 = f.getUint(row, i++);
            questDirectionId01 = f.getUint(row, i++);
            questDirectionId02 = f.getUint(row, i++);
            questDirectionId03 = f.getUint(row, i++);
            questDirectionId04 = f.getUint(row, i++);
            questDirectionId05 = f.getUint(row, i++);
            questDirectionId06 = f.getUint(row, i++);
            questDirectionId07 = f.getUint(row, i++);
            characterTitleIdReward = f.getUint(row, i++);
        }
    };
}
