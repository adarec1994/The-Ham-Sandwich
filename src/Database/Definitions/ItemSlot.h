#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct ItemSlot
    {
        uint32_t ID;
        std::wstring EnumName;
        uint32_t equippedSlotFlags;
        float armorModifier;
        float itemLevelModifier;
        uint32_t slotBonus;
        uint32_t glyphSlotBonus;
        uint32_t minLevel;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            EnumName = file.getString(recordIndex, i++);
            equippedSlotFlags = file.getUint(recordIndex, i++);
            armorModifier = file.getFloat(recordIndex, i++);
            itemLevelModifier = file.getFloat(recordIndex, i++);
            slotBonus = file.getUint(recordIndex, i++);
            glyphSlotBonus = file.getUint(recordIndex, i++);
            minLevel = file.getUint(recordIndex, i++);
        }
    };
}
