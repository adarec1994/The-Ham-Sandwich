#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct WordFilter
    {
        static constexpr const char* GetFileName() { return "WordFilter"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        std::wstring filter;
        uint32_t userTextFilterClassEnum;
        uint32_t deploymentRegionEnum;
        uint32_t languageId;
        uint32_t wordFilterTypeEnum;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            filter = file.getString(recordIndex, col++);
            userTextFilterClassEnum = file.getUint(recordIndex, col++);
            deploymentRegionEnum = file.getUint(recordIndex, col++);
            languageId = file.getUint(recordIndex, col++);
            wordFilterTypeEnum = file.getUint(recordIndex, col++);
        }
    };
}