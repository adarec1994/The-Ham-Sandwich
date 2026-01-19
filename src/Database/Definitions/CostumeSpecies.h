#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct CostumeSpecies
    {
        uint32_t ID;
        uint32_t componentLayoutId;
        std::wstring enumName;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            componentLayoutId = file.getUint(recordIndex, i++);
            enumName = file.getString(recordIndex, i++);
        }
    };
}
