#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TutorialAnchor
    {
        static constexpr const char* GetFileName() { return "TutorialAnchor"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t tutorialAnchorOrientationEnum;
        uint32_t hOffset;
        uint32_t vOffset;
        uint32_t flags;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            tutorialAnchorOrientationEnum = file.getUint(recordIndex, col++);
            hOffset = file.getUint(recordIndex, col++);
            vOffset = file.getUint(recordIndex, col++);
            flags = file.getUint(recordIndex, col++);
        }
    };
}