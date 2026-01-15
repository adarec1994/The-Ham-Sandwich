#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PvPRatingFloor
    {
        uint32_t ID;
        uint32_t flags;
        uint32_t pvpRatingTypeEnum;
        uint32_t floorValue;
        uint32_t localizedTextIdLabel;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            flags = f.getUint(row, i++);
            pvpRatingTypeEnum = f.getUint(row, i++);
            floorValue = f.getUint(row, i++);
            localizedTextIdLabel = f.getUint(row, i++);
        }
    };
}
