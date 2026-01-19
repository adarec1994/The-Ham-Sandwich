#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct ItemImbuement
    {
        uint32_t ID;
        uint32_t quest2Id00;
        uint32_t quest2Id01;
        uint32_t quest2Id02;
        uint32_t quest2Id03;
        uint32_t quest2Id04;
        uint32_t quest2Id05;
        uint32_t quest2Id06;
        uint32_t quest2Id07;
        uint32_t quest2Id08;
        uint32_t quest2Id09;
        uint32_t quest2Id10;
        uint32_t quest2Id11;
        uint32_t quest2Id12;
        uint32_t quest2Id13;
        uint32_t quest2Id14;
        uint32_t itemImbuementRewardId00;
        uint32_t itemImbuementRewardId01;
        uint32_t itemImbuementRewardId02;
        uint32_t itemImbuementRewardId03;
        uint32_t itemImbuementRewardId04;
        uint32_t itemImbuementRewardId05;
        uint32_t itemImbuementRewardId06;
        uint32_t itemImbuementRewardId07;
        uint32_t itemImbuementRewardId08;
        uint32_t itemImbuementRewardId09;
        uint32_t itemImbuementRewardId10;
        uint32_t itemImbuementRewardId11;
        uint32_t itemImbuementRewardId12;
        uint32_t itemImbuementRewardId13;
        uint32_t itemImbuementRewardId14;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            quest2Id00 = file.getUint(recordIndex, i++);
            quest2Id01 = file.getUint(recordIndex, i++);
            quest2Id02 = file.getUint(recordIndex, i++);
            quest2Id03 = file.getUint(recordIndex, i++);
            quest2Id04 = file.getUint(recordIndex, i++);
            quest2Id05 = file.getUint(recordIndex, i++);
            quest2Id06 = file.getUint(recordIndex, i++);
            quest2Id07 = file.getUint(recordIndex, i++);
            quest2Id08 = file.getUint(recordIndex, i++);
            quest2Id09 = file.getUint(recordIndex, i++);
            quest2Id10 = file.getUint(recordIndex, i++);
            quest2Id11 = file.getUint(recordIndex, i++);
            quest2Id12 = file.getUint(recordIndex, i++);
            quest2Id13 = file.getUint(recordIndex, i++);
            quest2Id14 = file.getUint(recordIndex, i++);
            itemImbuementRewardId00 = file.getUint(recordIndex, i++);
            itemImbuementRewardId01 = file.getUint(recordIndex, i++);
            itemImbuementRewardId02 = file.getUint(recordIndex, i++);
            itemImbuementRewardId03 = file.getUint(recordIndex, i++);
            itemImbuementRewardId04 = file.getUint(recordIndex, i++);
            itemImbuementRewardId05 = file.getUint(recordIndex, i++);
            itemImbuementRewardId06 = file.getUint(recordIndex, i++);
            itemImbuementRewardId07 = file.getUint(recordIndex, i++);
            itemImbuementRewardId08 = file.getUint(recordIndex, i++);
            itemImbuementRewardId09 = file.getUint(recordIndex, i++);
            itemImbuementRewardId10 = file.getUint(recordIndex, i++);
            itemImbuementRewardId11 = file.getUint(recordIndex, i++);
            itemImbuementRewardId12 = file.getUint(recordIndex, i++);
            itemImbuementRewardId13 = file.getUint(recordIndex, i++);
            itemImbuementRewardId14 = file.getUint(recordIndex, i++);
        }
    };
}
