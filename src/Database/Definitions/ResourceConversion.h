#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct ResourceConversion
    {
        uint32_t ID;
        uint32_t resourceConversionTypeEnum;
        uint32_t sourceId;
        uint32_t sourceCount;
        uint32_t targetId;
        uint32_t targetCount;
        uint32_t surchargeId;
        uint32_t surchargeCount;
        uint32_t flags;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            resourceConversionTypeEnum = f.getUint(row, i++);
            sourceId = f.getUint(row, i++);
            sourceCount = f.getUint(row, i++);
            targetId = f.getUint(row, i++);
            targetCount = f.getUint(row, i++);
            surchargeId = f.getUint(row, i++);
            surchargeCount = f.getUint(row, i++);
            flags = f.getUint(row, i++);
        }
    };
}
