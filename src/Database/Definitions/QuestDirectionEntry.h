#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct QuestDirectionEntry
    {
        uint32_t ID;
        uint32_t worldLocation2Id;
        uint32_t worldLocation2IdInactive;
        uint32_t worldZoneId;
        uint32_t questDirectionEntryFlags;
        uint32_t questDirectionFactionEnum;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            worldLocation2Id = f.getUint(row, i++);
            worldLocation2IdInactive = f.getUint(row, i++);
            worldZoneId = f.getUint(row, i++);
            questDirectionEntryFlags = f.getUint(row, i++);
            questDirectionFactionEnum = f.getUint(row, i++);
        }
    };
}
