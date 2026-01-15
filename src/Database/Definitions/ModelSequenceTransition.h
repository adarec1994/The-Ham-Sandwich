#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct ModelSequenceTransition
    {
        uint32_t ID;
        uint32_t modelSequenceIdFrom;
        uint32_t modelSequenceIdTo;
        uint32_t modelSequenceIdTransition;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            modelSequenceIdFrom = file.getUint(recordIndex, i++);
            modelSequenceIdTo = file.getUint(recordIndex, i++);
            modelSequenceIdTransition = file.getUint(recordIndex, i++);
        }
    };
}
