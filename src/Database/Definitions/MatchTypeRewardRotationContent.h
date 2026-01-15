#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct MatchTypeRewardRotationContent
    {
        uint32_t ID;
        uint32_t matchTypeEnum;
        uint32_t rewardRotationContentIdRandomNormal;
        uint32_t rewardRotationContentIdRandomVeteran;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            matchTypeEnum = file.getUint(recordIndex, i++);
            rewardRotationContentIdRandomNormal = file.getUint(recordIndex, i++);
            rewardRotationContentIdRandomVeteran = file.getUint(recordIndex, i++);
        }
    };
}
