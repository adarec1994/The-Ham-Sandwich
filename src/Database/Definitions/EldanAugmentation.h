#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct EldanAugmentation
    {
        uint32_t ID;
        uint32_t displayRow;
        uint32_t displayColumn;
        uint32_t classId;
        uint32_t powerCost;
        uint32_t eldanAugmentationIdRequired;
        uint32_t spell4IdAugment;
        uint32_t item2IdUnlock;
        uint32_t eldanAugmentationCategoryId;
        uint32_t categoryTier;
        uint32_t localizedTextIdTitle;
        uint32_t localizedTextIdTooltip;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            displayRow = file.getUint(recordIndex, i++);
            displayColumn = file.getUint(recordIndex, i++);
            classId = file.getUint(recordIndex, i++);
            powerCost = file.getUint(recordIndex, i++);
            eldanAugmentationIdRequired = file.getUint(recordIndex, i++);
            spell4IdAugment = file.getUint(recordIndex, i++);
            item2IdUnlock = file.getUint(recordIndex, i++);
            eldanAugmentationCategoryId = file.getUint(recordIndex, i++);
            categoryTier = file.getUint(recordIndex, i++);
            localizedTextIdTitle = file.getUint(recordIndex, i++);
            localizedTextIdTooltip = file.getUint(recordIndex, i++);
        }
    };
}
