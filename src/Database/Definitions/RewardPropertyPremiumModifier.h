#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct RewardPropertyPremiumModifier
    {
        uint32_t ID;
        uint32_t premiumSystemEnum;
        uint32_t tier;
        uint32_t rewardPropertyId;
        uint32_t rewardPropertyData;
        uint32_t modifierValueInt;
        float modifierValueFloat;
        uint32_t entitlementIdModifierCount;
        uint32_t flags;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            premiumSystemEnum = f.getUint(row, i++);
            tier = f.getUint(row, i++);
            rewardPropertyId = f.getUint(row, i++);
            rewardPropertyData = f.getUint(row, i++);
            modifierValueInt = f.getUint(row, i++);
            modifierValueFloat = f.getFloat(row, i++);
            entitlementIdModifierCount = f.getUint(row, i++);
            flags = f.getUint(row, i++);
        }
    };
}
