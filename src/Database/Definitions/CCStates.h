#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct CCStates
    {
        uint32_t ID;
        uint32_t flags;
        float defaultBreakProbability;
        uint32_t localizedTextIdName;
        std::wstring spellIcon;
        uint32_t visualEffectId[3];
        uint32_t ccStateDiminishingReturnsId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            defaultBreakProbability = file.getFloat(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            spellIcon = file.getString(recordIndex, i++);
            for (int j = 0; j < 3; ++j)
                visualEffectId[j] = file.getUint(recordIndex, i++);
            ccStateDiminishingReturnsId = file.getUint(recordIndex, i++);
        }
    };
}
