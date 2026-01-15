#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PublicEventUnitPropertyModifier
    {
        uint32_t ID;
        uint32_t publicEventId;
        uint32_t unitProperty2Id;
        float scalar;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            publicEventId = f.getUint(row, i++);
            unitProperty2Id = f.getUint(row, i++);
            scalar = f.getFloat(row, i++);
        }
    };
}
