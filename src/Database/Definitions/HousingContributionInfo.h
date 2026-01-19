#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct HousingContributionInfo
    {
        uint32_t ID;
        uint32_t housingContributionTypeId;
        uint32_t contributionPointRequirement;
        uint32_t item2IdTier00;
        uint32_t item2IdTier01;
        uint32_t item2IdTier02;
        uint32_t item2IdTier03;
        uint32_t item2IdTier04;
        uint32_t contributionPointValueTier00;
        uint32_t contributionPointValueTier01;
        uint32_t contributionPointValueTier02;
        uint32_t contributionPointValueTier03;
        uint32_t contributionPointValueTier04;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            housingContributionTypeId = file.getUint(recordIndex, i++);
            contributionPointRequirement = file.getUint(recordIndex, i++);
            item2IdTier00 = file.getUint(recordIndex, i++);
            item2IdTier01 = file.getUint(recordIndex, i++);
            item2IdTier02 = file.getUint(recordIndex, i++);
            item2IdTier03 = file.getUint(recordIndex, i++);
            item2IdTier04 = file.getUint(recordIndex, i++);
            contributionPointValueTier00 = file.getUint(recordIndex, i++);
            contributionPointValueTier01 = file.getUint(recordIndex, i++);
            contributionPointValueTier02 = file.getUint(recordIndex, i++);
            contributionPointValueTier03 = file.getUint(recordIndex, i++);
            contributionPointValueTier04 = file.getUint(recordIndex, i++);
        }
    };
}
