#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct GenericUnlockEntry
    {
        uint32_t ID;
        uint32_t localizedTextIdDescription;
        std::wstring spriteIcon;
        std::wstring spritePreview;
        uint32_t genericUnlockTypeEnum;
        uint32_t unlockObject;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdDescription = file.getUint(recordIndex, i++);
            spriteIcon = file.getString(recordIndex, i++);
            spritePreview = file.getString(recordIndex, i++);
            genericUnlockTypeEnum = file.getUint(recordIndex, i++);
            unlockObject = file.getUint(recordIndex, i++);
        }
    };
}
