#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct EmoteSequenceTransition
    {
        uint32_t ID;
        uint32_t emotesIdTo;
        uint32_t standStateFrom;
        uint32_t emotesIdFrom;
        uint32_t modelSequenceId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            emotesIdTo = file.getUint(recordIndex, i++);
            standStateFrom = file.getUint(recordIndex, i++);
            emotesIdFrom = file.getUint(recordIndex, i++);
            modelSequenceId = file.getUint(recordIndex, i++);
        }
    };
}
