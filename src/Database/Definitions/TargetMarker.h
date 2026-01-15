#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TargetMarker
    {
        static constexpr const char* GetFileName() { return "TargetMarker"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t visualEffectId;
        std::wstring iconPath;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            visualEffectId = file.getUint(recordIndex, col++);
            iconPath = file.getString(recordIndex, col++);
        }
    };
}