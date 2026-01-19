#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct GenericUnlockSet
    {
        uint32_t ID;
        uint32_t genericUnlockScopeEnum;
        uint32_t genericUnlockEntryId00;
        uint32_t genericUnlockEntryId01;
        uint32_t genericUnlockEntryId02;
        uint32_t genericUnlockEntryId03;
        uint32_t genericUnlockEntryId04;
        uint32_t genericUnlockEntryId05;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            genericUnlockScopeEnum = file.getUint(recordIndex, i++);
            genericUnlockEntryId00 = file.getUint(recordIndex, i++);
            genericUnlockEntryId01 = file.getUint(recordIndex, i++);
            genericUnlockEntryId02 = file.getUint(recordIndex, i++);
            genericUnlockEntryId03 = file.getUint(recordIndex, i++);
            genericUnlockEntryId04 = file.getUint(recordIndex, i++);
            genericUnlockEntryId05 = file.getUint(recordIndex, i++);
        }
    };
}
