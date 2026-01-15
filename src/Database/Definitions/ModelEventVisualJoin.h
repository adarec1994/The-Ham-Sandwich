#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct ModelEventVisualJoin
    {
        uint32_t ID;
        uint32_t unitVisualTypeId;
        uint32_t itemVisualTypeId;
        uint32_t materialTypeId;
        uint32_t modelEventId;
        uint32_t visualEffectId;
        uint32_t modelSequenceId;
        uint32_t flags;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            unitVisualTypeId = file.getUint(recordIndex, i++);
            itemVisualTypeId = file.getUint(recordIndex, i++);
            materialTypeId = file.getUint(recordIndex, i++);
            modelEventId = file.getUint(recordIndex, i++);
            visualEffectId = file.getUint(recordIndex, i++);
            modelSequenceId = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
        }
    };
}
