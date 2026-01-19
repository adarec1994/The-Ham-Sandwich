#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct AccountItem
    {
        uint32_t ID;
        uint32_t flags;
        uint32_t item2Id;
        uint32_t entitlementId;
        uint32_t entitlementCount;
        uint32_t entitlementScopeEnum;
        uint32_t instantEventEnum;
        uint32_t accountCurrencyEnum;
        uint64_t accountCurrencyAmount;
        std::wstring buttonIcon;
        uint32_t prerequisiteId;
        uint32_t accountItemCooldownGroupId;
        uint32_t storeDisplayInfoId;
        std::wstring storeIdentifierUpsell;
        uint32_t creature2DisplayGroupIdGacha;
        uint32_t entitlementIdPurchase;
        uint32_t genericUnlockSetId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            item2Id = file.getUint(recordIndex, i++);
            entitlementId = file.getUint(recordIndex, i++);
            entitlementCount = file.getUint(recordIndex, i++);
            entitlementScopeEnum = file.getUint(recordIndex, i++);
            instantEventEnum = file.getUint(recordIndex, i++);
            accountCurrencyEnum = file.getUint(recordIndex, i++);
            accountCurrencyAmount = static_cast<uint64_t>(file.getInt64(recordIndex, i++));
            buttonIcon = file.getString(recordIndex, i++);
            prerequisiteId = file.getUint(recordIndex, i++);
            accountItemCooldownGroupId = file.getUint(recordIndex, i++);
            storeDisplayInfoId = file.getUint(recordIndex, i++);
            storeIdentifierUpsell = file.getString(recordIndex, i++);
            creature2DisplayGroupIdGacha = file.getUint(recordIndex, i++);
            entitlementIdPurchase = file.getUint(recordIndex, i++);
            genericUnlockSetId = file.getUint(recordIndex, i++);
        }
    };
}
