#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct Tutorial
    {
        static constexpr const char* GetFileName() { return "Tutorial"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t flags;
        uint32_t tutorialCategoryEnum;
        uint32_t localizedTextIdContextualPopup;
        uint32_t tutorialAnchorId;
        uint32_t requiredLevel;
        uint32_t prerequisiteId;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            flags = file.getUint(recordIndex, col++);
            tutorialCategoryEnum = file.getUint(recordIndex, col++);
            localizedTextIdContextualPopup = file.getUint(recordIndex, col++);
            tutorialAnchorId = file.getUint(recordIndex, col++);
            requiredLevel = file.getUint(recordIndex, col++);
            prerequisiteId = file.getUint(recordIndex, col++);
        }
    };
}