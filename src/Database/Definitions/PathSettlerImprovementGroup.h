#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct PathSettlerImprovementGroup
    {
        uint32_t ID;
        uint32_t pathSettlerHubId;
        uint32_t pathSettlerImprovementGroupFlags;
        uint32_t creature2IdDepot;
        uint32_t localizedTextIdName;
        uint32_t settlerAvenueTypeEnum;
        uint32_t contributionValue;
        uint32_t perTierBonusContributionValue;
        uint32_t durationPerBundleMs;
        uint32_t maxBundleCount;
        uint32_t pathSettlerImprovementGroupIdOutpostRequired;
        uint32_t pathSettlerImprovementIdTier00;
        uint32_t pathSettlerImprovementIdTier01;
        uint32_t pathSettlerImprovementIdTier02;
        uint32_t pathSettlerImprovementIdTier03;
        uint32_t worldLocation2IdDisplayPoint;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            pathSettlerHubId = file.getUint(recordIndex, i++);
            pathSettlerImprovementGroupFlags = file.getUint(recordIndex, i++);
            creature2IdDepot = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            settlerAvenueTypeEnum = file.getUint(recordIndex, i++);
            contributionValue = file.getUint(recordIndex, i++);
            perTierBonusContributionValue = file.getUint(recordIndex, i++);
            durationPerBundleMs = file.getUint(recordIndex, i++);
            maxBundleCount = file.getUint(recordIndex, i++);
            pathSettlerImprovementGroupIdOutpostRequired = file.getUint(recordIndex, i++);
            pathSettlerImprovementIdTier00 = file.getUint(recordIndex, i++);
            pathSettlerImprovementIdTier01 = file.getUint(recordIndex, i++);
            pathSettlerImprovementIdTier02 = file.getUint(recordIndex, i++);
            pathSettlerImprovementIdTier03 = file.getUint(recordIndex, i++);
            worldLocation2IdDisplayPoint = file.getUint(recordIndex, i++);
        }
    };
}
