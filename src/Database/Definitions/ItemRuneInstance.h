#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct ItemRuneInstance
    {
        uint32_t ID;
        uint32_t definedSocketCount;
        uint32_t definedSocketType00;
        uint32_t definedSocketType01;
        uint32_t definedSocketType02;
        uint32_t definedSocketType03;
        uint32_t definedSocketType04;
        uint32_t definedSocketType05;
        uint32_t definedSocketType06;
        uint32_t definedSocketType07;
        uint32_t itemSetId;
        uint32_t itemSetPower;
        uint32_t socketCountMax;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            definedSocketCount = file.getUint(recordIndex, i++);
            definedSocketType00 = file.getUint(recordIndex, i++);
            definedSocketType01 = file.getUint(recordIndex, i++);
            definedSocketType02 = file.getUint(recordIndex, i++);
            definedSocketType03 = file.getUint(recordIndex, i++);
            definedSocketType04 = file.getUint(recordIndex, i++);
            definedSocketType05 = file.getUint(recordIndex, i++);
            definedSocketType06 = file.getUint(recordIndex, i++);
            definedSocketType07 = file.getUint(recordIndex, i++);
            itemSetId = file.getUint(recordIndex, i++);
            itemSetPower = file.getUint(recordIndex, i++);
            socketCountMax = file.getUint(recordIndex, i++);
        }
    };
}
