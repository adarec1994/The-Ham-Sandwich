#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct ClientEvent
    {
        uint32_t ID;
        std::wstring description;
        uint32_t worldId;
        uint32_t eventTypeEnum;
        uint32_t eventData;
        uint32_t prerequisiteId;
        uint32_t priority;
        uint32_t delayMS;
        uint32_t clientEventActionId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            description = file.getString(recordIndex, i++);
            worldId = file.getUint(recordIndex, i++);
            eventTypeEnum = file.getUint(recordIndex, i++);
            eventData = file.getUint(recordIndex, i++);
            prerequisiteId = file.getUint(recordIndex, i++);
            priority = file.getUint(recordIndex, i++);
            delayMS = file.getUint(recordIndex, i++);
            clientEventActionId = file.getUint(recordIndex, i++);
        }
    };
}
