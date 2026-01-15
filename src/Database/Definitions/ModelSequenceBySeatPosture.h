#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct ModelSequenceBySeatPosture
    {
        uint32_t ID;
        uint32_t modelSequenceId;
        uint32_t modelSequenceIdNarrow;
        uint32_t modelSequenceIdMedium;
        uint32_t modelSequenceIdWide;
        uint32_t modelSequenceIdBike;
        uint32_t modelSequenceIdHoverBoard;
        uint32_t modelSequenceIdGlider;
        uint32_t modelSequenceIdTank;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            modelSequenceId = file.getUint(recordIndex, i++);
            modelSequenceIdNarrow = file.getUint(recordIndex, i++);
            modelSequenceIdMedium = file.getUint(recordIndex, i++);
            modelSequenceIdWide = file.getUint(recordIndex, i++);
            modelSequenceIdBike = file.getUint(recordIndex, i++);
            modelSequenceIdHoverBoard = file.getUint(recordIndex, i++);
            modelSequenceIdGlider = file.getUint(recordIndex, i++);
            modelSequenceIdTank = file.getUint(recordIndex, i++);
        }
    };
}
