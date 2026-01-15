#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct LootSpell
    {
        uint32_t ID;
        uint32_t lootSpellTypeEnum;
        uint32_t lootSpellPickupEnumFlags;
        uint32_t creature2Id;
        std::wstring buttonIcon;
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdDescription;
        uint32_t visualEffectId;
        uint32_t data;
        uint32_t dataValue;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            lootSpellTypeEnum = file.getUint(recordIndex, i++);
            lootSpellPickupEnumFlags = file.getUint(recordIndex, i++);
            creature2Id = file.getUint(recordIndex, i++);
            buttonIcon = file.getString(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            localizedTextIdDescription = file.getUint(recordIndex, i++);
            visualEffectId = file.getUint(recordIndex, i++);
            data = file.getUint(recordIndex, i++);
            dataValue = file.getUint(recordIndex, i++);
        }
    };
}
