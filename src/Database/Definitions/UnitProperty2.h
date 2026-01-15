#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct UnitProperty2
    {
        static constexpr const char* GetFileName() { return "UnitProperty2"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        std::wstring description;
        std::wstring enumName;
        float defaultValue;
        uint32_t localizedTextId;
        float valuePerPoint;
        uint32_t flags;
        uint32_t tooltipDisplayOrder;
        uint32_t profiencyFlagEnum;
        uint32_t itemCraftingGroupFlagBitMask;
        uint32_t equippedSlotFlags;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            description = file.getString(recordIndex, col++);
            enumName = file.getString(recordIndex, col++);
            defaultValue = file.getFloat(recordIndex, col++);
            localizedTextId = file.getUint(recordIndex, col++);
            valuePerPoint = file.getFloat(recordIndex, col++);
            flags = file.getUint(recordIndex, col++);
            tooltipDisplayOrder = file.getUint(recordIndex, col++);
            profiencyFlagEnum = file.getUint(recordIndex, col++);
            itemCraftingGroupFlagBitMask = file.getUint(recordIndex, col++);
            equippedSlotFlags = file.getUint(recordIndex, col++);
        }
    };
}