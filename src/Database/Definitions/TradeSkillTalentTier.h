#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TradeskillTalentTier
    {
        static constexpr const char* GetFileName() { return "TradeskillTalentTier"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t tradeSkillId;
        uint32_t pointsToUnlock;
        uint32_t respecCost;
        uint32_t tradeSkillBonusId00;
        uint32_t tradeSkillBonusId01;
        uint32_t tradeSkillBonusId02;
        uint32_t tradeSkillBonusId03;
        uint32_t tradeSkillBonusId04;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            tradeSkillId = file.getUint(recordIndex, col++);
            pointsToUnlock = file.getUint(recordIndex, col++);
            respecCost = file.getUint(recordIndex, col++);
            tradeSkillBonusId00 = file.getUint(recordIndex, col++);
            tradeSkillBonusId01 = file.getUint(recordIndex, col++);
            tradeSkillBonusId02 = file.getUint(recordIndex, col++);
            tradeSkillBonusId03 = file.getUint(recordIndex, col++);
            tradeSkillBonusId04 = file.getUint(recordIndex, col++);
        }
    };
}