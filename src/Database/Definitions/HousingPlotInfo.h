#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct HousingPlotInfo
    {
        uint32_t ID;
        uint32_t worldSocketId;
        uint32_t plotType;
        uint32_t housingPropertyInfoId;
        uint32_t housingPropertyPlotIndex;
        uint32_t housingPlugItemIdDefault;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            worldSocketId = file.getUint(recordIndex, i++);
            plotType = file.getUint(recordIndex, i++);
            housingPropertyInfoId = file.getUint(recordIndex, i++);
            housingPropertyPlotIndex = file.getUint(recordIndex, i++);
            housingPlugItemIdDefault = file.getUint(recordIndex, i++);
        }
    };
}
