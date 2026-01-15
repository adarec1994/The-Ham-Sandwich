#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct QuestCategory
    {
        uint32_t ID;
        std::wstring description;
        uint32_t localizedTextIdTitle;
        uint32_t questCategoryTypeEnum;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            description = f.getString(row, i++);
            localizedTextIdTitle = f.getUint(row, i++);
            questCategoryTypeEnum = f.getUint(row, i++);
        }
    };
}
