#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct HousingContributionType
    {
        uint32_t ID;
        std::wstring description;
        std::wstring enumName;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            description = file.getString(recordIndex, i++);
            enumName = file.getString(recordIndex, i++);
        }
    };
}
