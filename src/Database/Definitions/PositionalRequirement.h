#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PositionalRequirement
    {
        uint32_t ID;
        uint32_t angleCenter;
        uint32_t angleRange;
        uint32_t flags;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            angleCenter = f.getUint(row, i++);
            angleRange = f.getUint(row, i++);
            flags = f.getUint(row, i++);
        }
    };
}
