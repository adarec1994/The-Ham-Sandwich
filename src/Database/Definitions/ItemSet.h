#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct ItemSet
    {
        uint32_t ID;
        uint32_t localizedTextId;
        uint32_t itemSetBonusId00;
        uint32_t itemSetBonusId01;
        uint32_t itemSetBonusId02;
        uint32_t itemSetBonusId03;
        uint32_t itemSetBonusId04;
        uint32_t itemSetBonusId05;
        uint32_t itemSetBonusId06;
        uint32_t itemSetBonusId07;
        uint32_t itemSetBonusId08;
        uint32_t itemSetBonusId09;
        uint32_t itemSetBonusId10;
        uint32_t itemSetBonusId11;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextId = file.getUint(recordIndex, i++);
            itemSetBonusId00 = file.getUint(recordIndex, i++);
            itemSetBonusId01 = file.getUint(recordIndex, i++);
            itemSetBonusId02 = file.getUint(recordIndex, i++);
            itemSetBonusId03 = file.getUint(recordIndex, i++);
            itemSetBonusId04 = file.getUint(recordIndex, i++);
            itemSetBonusId05 = file.getUint(recordIndex, i++);
            itemSetBonusId06 = file.getUint(recordIndex, i++);
            itemSetBonusId07 = file.getUint(recordIndex, i++);
            itemSetBonusId08 = file.getUint(recordIndex, i++);
            itemSetBonusId09 = file.getUint(recordIndex, i++);
            itemSetBonusId10 = file.getUint(recordIndex, i++);
            itemSetBonusId11 = file.getUint(recordIndex, i++);
        }
    };
}
