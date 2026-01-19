#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct AchievementChecklist
    {
        uint32_t ID;
        uint32_t achievementId;
        uint32_t bit;
        uint32_t objectId;
        uint32_t objectIdAlt;
        uint32_t prerequisiteId;
        uint32_t prerequisiteIdAlt;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            achievementId = file.getUint(recordIndex, i++);
            bit = file.getUint(recordIndex, i++);
            objectId = file.getUint(recordIndex, i++);
            objectIdAlt = file.getUint(recordIndex, i++);
            prerequisiteId = file.getUint(recordIndex, i++);
            prerequisiteIdAlt = file.getUint(recordIndex, i++);
        }
    };
}
