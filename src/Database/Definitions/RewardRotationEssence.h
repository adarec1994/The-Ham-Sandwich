#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct RewardRotationEssence
    {
        uint32_t ID;
        uint32_t accountCurrencyTypeId;
        uint32_t minPlayerLevel;
        uint32_t worldDifficultyFlags;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            accountCurrencyTypeId = f.getUint(row, i++);
            minPlayerLevel = f.getUint(row, i++);
            worldDifficultyFlags = f.getUint(row, i++);
        }
    };
}
