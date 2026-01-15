#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct StoryPanel
    {
        static constexpr const char* GetFileName() { return "StoryPanel"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t localizedTextIdBody;
        uint32_t soundEventId;
        uint32_t windowTypeId;
        uint32_t durationMS;
        uint32_t prerequisiteId;
        uint32_t storyPanelStyleEnum;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            localizedTextIdBody = file.getUint(recordIndex, col++);
            soundEventId = file.getUint(recordIndex, col++);
            windowTypeId = file.getUint(recordIndex, col++);
            durationMS = file.getUint(recordIndex, col++);
            prerequisiteId = file.getUint(recordIndex, col++);
            storyPanelStyleEnum = file.getUint(recordIndex, col++);
        }
    };
}