#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct ModelSequenceByMode
    {
        uint32_t ID;
        uint32_t modelSequenceId;
        uint32_t modelSequenceIdSwim;
        uint32_t modelSequenceIdHover;
        uint32_t modelSequenceIdFly;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            modelSequenceId = file.getUint(recordIndex, i++);
            modelSequenceIdSwim = file.getUint(recordIndex, i++);
            modelSequenceIdHover = file.getUint(recordIndex, i++);
            modelSequenceIdFly = file.getUint(recordIndex, i++);
        }
    };
}
