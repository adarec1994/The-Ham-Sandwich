#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct CharacterTitle
    {
        uint32_t ID;
        uint32_t characterTitleCategoryId;
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdTitle;
        uint32_t spell4IdActivate;
        uint32_t lifeTimeSeconds;
        uint32_t playerTitleFlagsEnum;
        uint32_t scheduleId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            characterTitleCategoryId = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            localizedTextIdTitle = file.getUint(recordIndex, i++);
            spell4IdActivate = file.getUint(recordIndex, i++);
            lifeTimeSeconds = file.getUint(recordIndex, i++);
            playerTitleFlagsEnum = file.getUint(recordIndex, i++);
            scheduleId = file.getUint(recordIndex, i++);
        }
    };
}
