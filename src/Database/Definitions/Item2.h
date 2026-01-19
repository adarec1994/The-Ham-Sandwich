#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct Item2
    {
        uint32_t ID;
        uint32_t itemBudgetId;
        uint32_t itemStatId;
        uint32_t itemRuneInstanceId;
        uint32_t itemQualityId;
        uint32_t itemSpecialId00;
        uint32_t itemImbuementId;
        uint32_t item2FamilyId;
        uint32_t item2CategoryId;
        uint32_t item2TypeId;
        uint32_t itemDisplayId;
        uint32_t itemSourceId;
        uint32_t classRequired;
        uint32_t raceRequired;
        uint32_t faction2IdRequired;
        uint32_t powerLevel;
        uint32_t requiredLevel;
        uint32_t requiredItemLevel;
        uint32_t prerequisiteId;
        uint32_t equippedSlotFlags;
        uint32_t maxStackCount;
        uint32_t maxCharges;
        uint32_t expirationTimeMinutes;
        uint32_t quest2IdActivation;
        uint32_t quest2IdActivationRequired;
        uint32_t questObjectiveActivationRequired;
        uint32_t tradeskillAdditiveId;
        uint32_t tradeskillCatalystId;
        uint32_t housingDecorInfoId;
        uint32_t housingWarplotBossTokenId;
        uint32_t genericUnlockSetId;
        uint32_t flags;
        uint32_t bindFlags;
        uint32_t buyFromVendorStackCount;
        uint32_t currencyTypeId0;
        uint32_t currencyTypeId1;
        uint32_t currencyAmount0;
        uint32_t currencyAmount1;
        uint32_t currencyTypeId0SellToVendor;
        uint32_t currencyTypeId1SellToVendor;
        uint32_t currencyAmount0SellToVendor;
        uint32_t currencyAmount1SellToVendor;
        uint32_t itemColorSetId;
        float supportPowerPercentage;
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdTooltip;
        std::wstring buttonTemplate;
        std::wstring buttonIcon;
        uint32_t soundEventIdEquip;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            itemBudgetId = file.getUint(recordIndex, i++);
            itemStatId = file.getUint(recordIndex, i++);
            itemRuneInstanceId = file.getUint(recordIndex, i++);
            itemQualityId = file.getUint(recordIndex, i++);
            itemSpecialId00 = file.getUint(recordIndex, i++);
            itemImbuementId = file.getUint(recordIndex, i++);
            item2FamilyId = file.getUint(recordIndex, i++);
            item2CategoryId = file.getUint(recordIndex, i++);
            item2TypeId = file.getUint(recordIndex, i++);
            itemDisplayId = file.getUint(recordIndex, i++);
            itemSourceId = file.getUint(recordIndex, i++);
            classRequired = file.getUint(recordIndex, i++);
            raceRequired = file.getUint(recordIndex, i++);
            faction2IdRequired = file.getUint(recordIndex, i++);
            powerLevel = file.getUint(recordIndex, i++);
            requiredLevel = file.getUint(recordIndex, i++);
            requiredItemLevel = file.getUint(recordIndex, i++);
            prerequisiteId = file.getUint(recordIndex, i++);
            equippedSlotFlags = file.getUint(recordIndex, i++);
            maxStackCount = file.getUint(recordIndex, i++);
            maxCharges = file.getUint(recordIndex, i++);
            expirationTimeMinutes = file.getUint(recordIndex, i++);
            quest2IdActivation = file.getUint(recordIndex, i++);
            quest2IdActivationRequired = file.getUint(recordIndex, i++);
            questObjectiveActivationRequired = file.getUint(recordIndex, i++);
            tradeskillAdditiveId = file.getUint(recordIndex, i++);
            tradeskillCatalystId = file.getUint(recordIndex, i++);
            housingDecorInfoId = file.getUint(recordIndex, i++);
            housingWarplotBossTokenId = file.getUint(recordIndex, i++);
            genericUnlockSetId = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            bindFlags = file.getUint(recordIndex, i++);
            buyFromVendorStackCount = file.getUint(recordIndex, i++);
            currencyTypeId0 = file.getUint(recordIndex, i++);
            currencyTypeId1 = file.getUint(recordIndex, i++);
            currencyAmount0 = file.getUint(recordIndex, i++);
            currencyAmount1 = file.getUint(recordIndex, i++);
            currencyTypeId0SellToVendor = file.getUint(recordIndex, i++);
            currencyTypeId1SellToVendor = file.getUint(recordIndex, i++);
            currencyAmount0SellToVendor = file.getUint(recordIndex, i++);
            currencyAmount1SellToVendor = file.getUint(recordIndex, i++);
            itemColorSetId = file.getUint(recordIndex, i++);
            supportPowerPercentage = file.getFloat(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            localizedTextIdTooltip = file.getUint(recordIndex, i++);
            buttonTemplate = file.getString(recordIndex, i++);
            buttonIcon = file.getString(recordIndex, i++);
            soundEventIdEquip = file.getUint(recordIndex, i++);
        }
    };
}
