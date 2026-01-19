#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct GameFormula
    {
        uint32_t ID;
        uint32_t dataint0;
        uint32_t dataint01;
        uint32_t dataint02;
        uint32_t dataint03;
        uint32_t dataint04;
        float datafloat0;
        float datafloat01;
        float datafloat02;
        float datafloat03;
        float datafloat04;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            dataint0 = file.getUint(recordIndex, i++);
            dataint01 = file.getUint(recordIndex, i++);
            dataint02 = file.getUint(recordIndex, i++);
            dataint03 = file.getUint(recordIndex, i++);
            dataint04 = file.getUint(recordIndex, i++);
            datafloat0 = file.getFloat(recordIndex, i++);
            datafloat01 = file.getFloat(recordIndex, i++);
            datafloat02 = file.getFloat(recordIndex, i++);
            datafloat03 = file.getFloat(recordIndex, i++);
            datafloat04 = file.getFloat(recordIndex, i++);
        }
    };
}
