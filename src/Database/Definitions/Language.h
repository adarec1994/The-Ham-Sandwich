#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct Language
    {
        uint32_t ID;
        std::wstring languageTag;
        std::wstring clientLanguageTag;
        uint32_t soundAvailabilityIndexFemale;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            languageTag = file.getString(recordIndex, i++);
            clientLanguageTag = file.getString(recordIndex, i++);
            soundAvailabilityIndexFemale = file.getUint(recordIndex, i++);
        }
    };
}
