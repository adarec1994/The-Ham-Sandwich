#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct HousingPlugItem
    {
        uint32_t ID;
        uint32_t localizedTextIdName;
        uint32_t housingPlotTypeId;
        uint32_t localizedTextIdTooltip;
        uint32_t worldIdPlug00;
        uint32_t worldIdPlug01;
        uint32_t worldIdPlug02;
        uint32_t worldIdPlug03;
        uint32_t flags;
        uint32_t housingResourceIdProvided00;
        uint32_t housingResourceIdProvided01;
        uint32_t housingResourceIdProvided02;
        uint32_t housingResourceIdProvided03;
        uint32_t housingResourceIdProvided04;
        uint32_t housingResourceIdPrerequisite00;
        uint32_t housingResourceIdPrerequisite01;
        uint32_t housingResourceIdPrerequisite02;
        uint32_t housingFeatureTypeFlags;
        uint32_t housingContributionInfoId00;
        uint32_t housingContributionInfoId01;
        uint32_t housingContributionInfoId02;
        uint32_t housingContributionInfoId03;
        uint32_t housingContributionInfoId04;
        uint32_t housingPlugItemIdNextUpgrade;
        uint32_t localizedTextIdPrereqTooltip00;
        uint32_t localizedTextIdPrereqTooltip01;
        uint32_t localizedTextIdPrereqTooltip02;
        uint32_t prerequisiteId00;
        uint32_t prerequisiteId01;
        uint32_t prerequisiteId02;
        uint32_t prerequisiteIdUnlock;
        uint32_t housingBuildId;
        uint32_t housingUpkeepTypeEnum;
        uint32_t upkeepCharges;
        float upkeepTime;
        uint32_t housingContributionInfoIdUpkeepCost00;
        uint32_t housingContributionInfoIdUpkeepCost01;
        uint32_t housingContributionInfoIdUpkeepCost02;
        uint32_t housingContributionInfoIdUpkeepCost03;
        uint32_t housingContributionInfoIdUpkeepCost04;
        uint32_t gameFormulaIdHousingBuildBonus;
        std::wstring screenshotSprite00;
        std::wstring screenshotSprite01;
        std::wstring screenshotSprite02;
        std::wstring screenshotSprite03;
        std::wstring screenshotSprite04;
        uint32_t housingPlugTypeEnum;
        uint32_t accountItemIdUpsell;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            housingPlotTypeId = file.getUint(recordIndex, i++);
            localizedTextIdTooltip = file.getUint(recordIndex, i++);
            worldIdPlug00 = file.getUint(recordIndex, i++);
            worldIdPlug01 = file.getUint(recordIndex, i++);
            worldIdPlug02 = file.getUint(recordIndex, i++);
            worldIdPlug03 = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            housingResourceIdProvided00 = file.getUint(recordIndex, i++);
            housingResourceIdProvided01 = file.getUint(recordIndex, i++);
            housingResourceIdProvided02 = file.getUint(recordIndex, i++);
            housingResourceIdProvided03 = file.getUint(recordIndex, i++);
            housingResourceIdProvided04 = file.getUint(recordIndex, i++);
            housingResourceIdPrerequisite00 = file.getUint(recordIndex, i++);
            housingResourceIdPrerequisite01 = file.getUint(recordIndex, i++);
            housingResourceIdPrerequisite02 = file.getUint(recordIndex, i++);
            housingFeatureTypeFlags = file.getUint(recordIndex, i++);
            housingContributionInfoId00 = file.getUint(recordIndex, i++);
            housingContributionInfoId01 = file.getUint(recordIndex, i++);
            housingContributionInfoId02 = file.getUint(recordIndex, i++);
            housingContributionInfoId03 = file.getUint(recordIndex, i++);
            housingContributionInfoId04 = file.getUint(recordIndex, i++);
            housingPlugItemIdNextUpgrade = file.getUint(recordIndex, i++);
            localizedTextIdPrereqTooltip00 = file.getUint(recordIndex, i++);
            localizedTextIdPrereqTooltip01 = file.getUint(recordIndex, i++);
            localizedTextIdPrereqTooltip02 = file.getUint(recordIndex, i++);
            prerequisiteId00 = file.getUint(recordIndex, i++);
            prerequisiteId01 = file.getUint(recordIndex, i++);
            prerequisiteId02 = file.getUint(recordIndex, i++);
            prerequisiteIdUnlock = file.getUint(recordIndex, i++);
            housingBuildId = file.getUint(recordIndex, i++);
            housingUpkeepTypeEnum = file.getUint(recordIndex, i++);
            upkeepCharges = file.getUint(recordIndex, i++);
            upkeepTime = file.getFloat(recordIndex, i++);
            housingContributionInfoIdUpkeepCost00 = file.getUint(recordIndex, i++);
            housingContributionInfoIdUpkeepCost01 = file.getUint(recordIndex, i++);
            housingContributionInfoIdUpkeepCost02 = file.getUint(recordIndex, i++);
            housingContributionInfoIdUpkeepCost03 = file.getUint(recordIndex, i++);
            housingContributionInfoIdUpkeepCost04 = file.getUint(recordIndex, i++);
            gameFormulaIdHousingBuildBonus = file.getUint(recordIndex, i++);
            screenshotSprite00 = file.getString(recordIndex, i++);
            screenshotSprite01 = file.getString(recordIndex, i++);
            screenshotSprite02 = file.getString(recordIndex, i++);
            screenshotSprite03 = file.getString(recordIndex, i++);
            screenshotSprite04 = file.getString(recordIndex, i++);
            housingPlugTypeEnum = file.getUint(recordIndex, i++);
            accountItemIdUpsell = file.getUint(recordIndex, i++);
        }
    };
}
