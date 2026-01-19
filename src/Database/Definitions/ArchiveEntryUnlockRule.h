#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct ArchiveEntryUnlockRule
    {
        uint32_t ID;
        uint32_t archiveEntryId;
        uint32_t archiveEntryUnlockRuleEnum;
        uint32_t archiveEntryUnlockRuleFlags;
        uint32_t object[6];

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            archiveEntryId = file.getUint(recordIndex, i++);
            archiveEntryUnlockRuleEnum = file.getUint(recordIndex, i++);
            archiveEntryUnlockRuleFlags = file.getUint(recordIndex, i++);
            for (int j = 0; j < 6; ++j)
                object[j] = file.getUint(recordIndex, i++);
        }
    };
}
