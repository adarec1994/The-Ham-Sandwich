#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct Item2Category
    {
        uint32_t ID;
        uint32_t localizedTextId;
        uint32_t itemProficiencyId;
        uint32_t flags;
        uint32_t tradeSkillId;
        uint32_t soundEventIdEquip;
        float vendorMultiplier;
        float turninMultiplier;
        float armorModifier;
        float armorBase;
        float weaponPowerModifier;
        uint32_t weaponPowerBase;
        uint32_t item2FamilyId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextId = file.getUint(recordIndex, i++);
            itemProficiencyId = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            tradeSkillId = file.getUint(recordIndex, i++);
            soundEventIdEquip = file.getUint(recordIndex, i++);
            vendorMultiplier = file.getFloat(recordIndex, i++);
            turninMultiplier = file.getFloat(recordIndex, i++);
            armorModifier = file.getFloat(recordIndex, i++);
            armorBase = file.getFloat(recordIndex, i++);
            weaponPowerModifier = file.getFloat(recordIndex, i++);
            weaponPowerBase = file.getUint(recordIndex, i++);
            item2FamilyId = file.getUint(recordIndex, i++);
        }
    };
}
