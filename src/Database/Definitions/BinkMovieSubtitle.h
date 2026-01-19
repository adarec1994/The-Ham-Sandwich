#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct BinkMovieSubtitle
    {
        uint32_t ID;
        uint32_t binkMovieId;
        uint32_t delayMs;
        uint32_t localizedTextIdDisplayText;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            binkMovieId = file.getUint(recordIndex, i++);
            delayMs = file.getUint(recordIndex, i++);
            localizedTextIdDisplayText = file.getUint(recordIndex, i++);
        }
    };
}
