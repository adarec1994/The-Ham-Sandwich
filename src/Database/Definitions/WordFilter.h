#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct WordFilter
    {
        uint32_t ID;
        std::wstring filter;
        uint32_t userTextFilterClassEnum;
        uint32_t deploymentRegionEnum;
        uint32_t languageId;
        uint32_t wordFilterTypeEnum;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            filter = file.getString(recordIndex, i++);
            userTextFilterClassEnum = file.getUint(recordIndex, i++);
            deploymentRegionEnum = file.getUint(recordIndex, i++);
            languageId = file.getUint(recordIndex, i++);
            wordFilterTypeEnum = file.getUint(recordIndex, i++);
        }
    };
}
