#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Redacted
    {
        uint32_t ID;
        std::wstring idString;
        uint32_t localizedTextIdName;
        uint32_t mtxCategoryIdParent;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            idString = f.getString(row, i++);
            localizedTextIdName = f.getUint(row, i++);
            mtxCategoryIdParent = f.getUint(row, i++);
        }
    };
}
