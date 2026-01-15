#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct SoundReplace
    {
        uint32_t ID;
        uint32_t soundReplaceDescriptionId;
        uint32_t soundEventIdOld;
        uint32_t soundEventIdNew;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            soundReplaceDescriptionId = f.getUint(row, i++);
            soundEventIdOld = f.getUint(row, i++);
            soundEventIdNew = f.getUint(row, i++);
        }
    };
}
