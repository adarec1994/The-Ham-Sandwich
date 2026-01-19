#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct CharacterCreation
    {
        uint32_t ID;
        uint32_t classId;
        uint32_t raceId;
        uint32_t sex;
        uint32_t factionId;
        uint32_t costumeOnly;
        uint32_t itemId[16];
        uint32_t enabled;
        uint32_t characterCreationStartEnum;
        uint32_t xp;
        uint32_t accountCurrencyTypeIdCost;
        uint32_t accountCurrencyAmountCost;
        uint32_t entitlementIdRequired;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            classId = file.getUint(recordIndex, i++);
            raceId = file.getUint(recordIndex, i++);
            sex = file.getUint(recordIndex, i++);
            factionId = file.getUint(recordIndex, i++);
            costumeOnly = file.getUint(recordIndex, i++);
            for (int j = 0; j < 16; ++j)
                itemId[j] = file.getUint(recordIndex, i++);
            enabled = file.getUint(recordIndex, i++);
            characterCreationStartEnum = file.getUint(recordIndex, i++);
            xp = file.getUint(recordIndex, i++);
            accountCurrencyTypeIdCost = file.getUint(recordIndex, i++);
            accountCurrencyAmountCost = file.getUint(recordIndex, i++);
            entitlementIdRequired = file.getUint(recordIndex, i++);
        }
    };
}
