#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct ComponentRegionRect
    {
        uint32_t ID;
        uint32_t regionId;
        uint32_t rectMinX;
        uint32_t rectMinY;
        uint32_t rectLimX;
        uint32_t rectLimY;
        uint32_t componentLayoutId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            regionId = file.getUint(recordIndex, i++);
            rectMinX = file.getUint(recordIndex, i++);
            rectMinY = file.getUint(recordIndex, i++);
            rectLimX = file.getUint(recordIndex, i++);
            rectLimY = file.getUint(recordIndex, i++);
            componentLayoutId = file.getUint(recordIndex, i++);
        }
    };
}
