#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TradeskillAchievementLayout
    {
        static constexpr const char* GetFileName() { return "TradeskillAchievementLayout"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t achievementId;
        uint32_t achievementIdParent00;
        uint32_t achievementIdParent01;
        uint32_t achievementIdParent02;
        uint32_t achievementIdParent03;
        uint32_t achievementIdParent04;
        uint32_t gridX;
        uint32_t gridY;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            achievementId = file.getUint(recordIndex, col++);
            achievementIdParent00 = file.getUint(recordIndex, col++);
            achievementIdParent01 = file.getUint(recordIndex, col++);
            achievementIdParent02 = file.getUint(recordIndex, col++);
            achievementIdParent03 = file.getUint(recordIndex, col++);
            achievementIdParent04 = file.getUint(recordIndex, col++);
            gridX = file.getUint(recordIndex, col++);
            gridY = file.getUint(recordIndex, col++);
        }
    };
}