#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct CCStateAdditionalData
    {
        uint32_t ID;
        uint32_t ccStatesId;
        uint32_t dataInt[5];
        float dataFloat[5];

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            ccStatesId = file.getUint(recordIndex, i++);
            for (int j = 0; j < 5; ++j)
                dataInt[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 5; ++j)
                dataFloat[j] = file.getFloat(recordIndex, i++);
        }
    };
}
