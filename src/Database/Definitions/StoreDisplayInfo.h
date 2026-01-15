#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct StoreDisplayInfo
    {
        static constexpr const char* GetFileName() { return "StoreDisplayInfo"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t displayType;
        uint32_t displayValue;
        uint32_t modelCameraId;
        uint32_t localizedTextIdName;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            displayType = file.getUint(recordIndex, col++);
            displayValue = file.getUint(recordIndex, col++);
            modelCameraId = file.getUint(recordIndex, col++);
            localizedTextIdName = file.getUint(recordIndex, col++);
        }
    };
}