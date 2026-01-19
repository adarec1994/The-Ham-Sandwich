#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct ActionBarShortcutSet
    {
        uint32_t ID;
        uint32_t shortcutType[12];
        uint32_t objectId[12];

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            for (int j = 0; j < 12; ++j)
                shortcutType[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 12; ++j)
                objectId[j] = file.getUint(recordIndex, i++);
        }
    };
}
