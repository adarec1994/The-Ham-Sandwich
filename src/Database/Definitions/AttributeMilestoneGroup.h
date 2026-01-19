#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct AttributeMilestoneGroup
    {
        uint32_t ID;
        uint32_t requiredAttributeAmount[10];

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            for (int j = 0; j < 10; ++j)
                requiredAttributeAmount[j] = file.getUint(recordIndex, i++);
        }
    };
}
