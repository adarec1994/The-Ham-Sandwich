#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Quest2RandomTextLineJoin
    {
        uint32_t ID;
        uint32_t quest2Id;
        uint32_t questVOTextType;
        uint32_t randomTextLineId;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            quest2Id = f.getUint(row, i++);
            questVOTextType = f.getUint(row, i++);
            randomTextLineId = f.getUint(row, i++);
        }
    };
}
