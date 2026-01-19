#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct Creature2Action
    {
        uint32_t ID;
        std::wstring description;
        uint32_t creatureActionSetId;
        uint32_t state;
        uint32_t wEvent;
        uint32_t orderIndex;
        uint32_t delayMS;
        uint32_t action;
        uint32_t actionData[2];
        uint32_t visualEffectId;
        uint32_t prerequisiteId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            description = file.getString(recordIndex, i++);
            creatureActionSetId = file.getUint(recordIndex, i++);
            state = file.getUint(recordIndex, i++);
            wEvent = file.getUint(recordIndex, i++);
            orderIndex = file.getUint(recordIndex, i++);
            delayMS = file.getUint(recordIndex, i++);
            action = file.getUint(recordIndex, i++);
            for (int j = 0; j < 2; ++j)
                actionData[j] = file.getUint(recordIndex, i++);
            visualEffectId = file.getUint(recordIndex, i++);
            prerequisiteId = file.getUint(recordIndex, i++);
        }
    };
}
