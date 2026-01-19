#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct EldanAugmentationCategory
    {
        uint32_t ID;
        uint32_t eldanAugmentationCategoryIdTier2Category00;
        uint32_t eldanAugmentationCategoryIdTier2Category01;
        uint32_t tier2CostAmount00;
        uint32_t tier2CostAmount01;
        uint32_t tier3CostAmount00;
        uint32_t tier3CostAmount01;
        uint32_t localizedTextIdName;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            eldanAugmentationCategoryIdTier2Category00 = file.getUint(recordIndex, i++);
            eldanAugmentationCategoryIdTier2Category01 = file.getUint(recordIndex, i++);
            tier2CostAmount00 = file.getUint(recordIndex, i++);
            tier2CostAmount01 = file.getUint(recordIndex, i++);
            tier3CostAmount00 = file.getUint(recordIndex, i++);
            tier3CostAmount01 = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
        }
    };
}
