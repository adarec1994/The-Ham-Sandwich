#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct Faction2
    {
        uint32_t ID;
        uint32_t faction2IdParent;
        uint32_t flags;
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdToolTip;
        uint32_t orderIndex;
        uint32_t archiveArticleId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            faction2IdParent = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            localizedTextIdToolTip = file.getUint(recordIndex, i++);
            orderIndex = file.getUint(recordIndex, i++);
            archiveArticleId = file.getUint(recordIndex, i++);
        }
    };
}
