#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct Entitlement
    {
        uint32_t ID;
        uint32_t maxCount;
        uint32_t flags;
        uint32_t spell4IdPersistentBuff;
        uint32_t characterTitleId;
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdDescription;
        std::wstring buttonIcon;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            maxCount = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            spell4IdPersistentBuff = file.getUint(recordIndex, i++);
            characterTitleId = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            localizedTextIdDescription = file.getUint(recordIndex, i++);
            buttonIcon = file.getString(recordIndex, i++);
        }
    };
}
