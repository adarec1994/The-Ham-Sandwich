#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TrackingSlot
    {
        static constexpr const char* GetFileName() { return "TrackingSlot"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t localizedTextIdLabel;
        std::wstring iconPath;
        uint32_t publicEventObjectiveId;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            localizedTextIdLabel = file.getUint(recordIndex, col++);
            iconPath = file.getString(recordIndex, col++);
            publicEventObjectiveId = file.getUint(recordIndex, col++);
        }
    };
}