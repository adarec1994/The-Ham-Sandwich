#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct LiveEvent
    {
        uint32_t ID;
        uint32_t liveEventTypeEnum;
        uint32_t maxValue;
        uint32_t flags;
        uint32_t liveEventCategoryEnum;
        uint32_t liveEventIdParent;
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdSummary;
        std::wstring iconPath;
        std::wstring iconPathButton;
        std::wstring spritePathTitle;
        std::wstring spritePathBackground;
        uint32_t currencyTypeIdEarned;
        uint32_t localizedTextIdCurrencyEarnedTooltip;
        uint32_t worldLocation2IdExile;
        uint32_t worldLocation2IdDominion;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            liveEventTypeEnum = file.getUint(recordIndex, i++);
            maxValue = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            liveEventCategoryEnum = file.getUint(recordIndex, i++);
            liveEventIdParent = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            localizedTextIdSummary = file.getUint(recordIndex, i++);
            iconPath = file.getString(recordIndex, i++);
            iconPathButton = file.getString(recordIndex, i++);
            spritePathTitle = file.getString(recordIndex, i++);
            spritePathBackground = file.getString(recordIndex, i++);
            currencyTypeIdEarned = file.getUint(recordIndex, i++);
            localizedTextIdCurrencyEarnedTooltip = file.getUint(recordIndex, i++);
            worldLocation2IdExile = file.getUint(recordIndex, i++);
            worldLocation2IdDominion = file.getUint(recordIndex, i++);
        }
    };
}
