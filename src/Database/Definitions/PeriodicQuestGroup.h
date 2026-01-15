#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PeriodicQuestGroup
    {
        uint32_t ID;
        uint32_t periodicQuestSetId;
        uint32_t periodicQuestsOffered;
        uint32_t maxPeriodicQuestsAllowed;
        uint32_t weight;
        uint32_t contractTypeEnum;
        uint32_t contractQualityEnum;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            periodicQuestSetId = f.getUint(row, i++);
            periodicQuestsOffered = f.getUint(row, i++);
            maxPeriodicQuestsAllowed = f.getUint(row, i++);
            weight = f.getUint(row, i++);
            contractTypeEnum = f.getUint(row, i++);
            contractQualityEnum = f.getUint(row, i++);
        }
    };
}
