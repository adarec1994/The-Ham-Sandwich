#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct LevelUpUnlock
    {
        uint32_t ID;
        uint32_t levelUpUnlockSystemEnum;
        uint32_t level;
        uint32_t levelUpUnlockTypeId;
        uint32_t localizedTextIdDescription;
        std::wstring displayIcon;
        uint32_t prerequisiteId;
        uint32_t levelUpUnlockValue;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            levelUpUnlockSystemEnum = file.getUint(recordIndex, i++);
            level = file.getUint(recordIndex, i++);
            levelUpUnlockTypeId = file.getUint(recordIndex, i++);
            localizedTextIdDescription = file.getUint(recordIndex, i++);
            displayIcon = file.getString(recordIndex, i++);
            prerequisiteId = file.getUint(recordIndex, i++);
            levelUpUnlockValue = file.getUint(recordIndex, i++);
        }
    };
}
