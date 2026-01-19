#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct AchievementCategory
    {
        uint32_t ID;
        uint32_t localizedTextId;
        uint32_t localizedTextIdFullName;
        uint32_t achievementCategoryIdParent;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextId = file.getUint(recordIndex, i++);
            localizedTextIdFullName = file.getUint(recordIndex, i++);
            achievementCategoryIdParent = file.getUint(recordIndex, i++);
        }
    };
}
