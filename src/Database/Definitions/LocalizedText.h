#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct LocalizedText
    {
        uint32_t ID;
        uint32_t soundEventId;
        uint32_t soundEventIdFemale;
        uint32_t version;
        uint32_t unitVoiceTypeId;
        uint32_t stringContextEnum;
        uint32_t soundAvailabilityFlagFemale;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            soundEventId = file.getUint(recordIndex, i++);
            soundEventIdFemale = file.getUint(recordIndex, i++);
            version = file.getUint(recordIndex, i++);
            unitVoiceTypeId = file.getUint(recordIndex, i++);
            stringContextEnum = file.getUint(recordIndex, i++);
            soundAvailabilityFlagFemale = file.getUint(recordIndex, i++);
        }
    };
}
