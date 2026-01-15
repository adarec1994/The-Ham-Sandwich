#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TradeskillAchievementReward
    {
        static constexpr const char* GetFileName() { return "TradeskillAchievementReward"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t achievementId;
        uint32_t faction2Id;
        uint32_t factionIdAmount;
        uint32_t talentPoints;
        uint32_t tradeSkillSchematicId00;
        uint32_t tradeSkillSchematicId01;
        uint32_t tradeSkillSchematicId02;
        uint32_t tradeSkillSchematicId03;
        uint32_t tradeSkillSchematicId04;
        uint32_t tradeSkillSchematicId05;
        uint32_t tradeSkillSchematicId06;
        uint32_t tradeSkillSchematicId07;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            achievementId = file.getUint(recordIndex, col++);
            faction2Id = file.getUint(recordIndex, col++);
            factionIdAmount = file.getUint(recordIndex, col++);
            talentPoints = file.getUint(recordIndex, col++);
            tradeSkillSchematicId00 = file.getUint(recordIndex, col++);
            tradeSkillSchematicId01 = file.getUint(recordIndex, col++);
            tradeSkillSchematicId02 = file.getUint(recordIndex, col++);
            tradeSkillSchematicId03 = file.getUint(recordIndex, col++);
            tradeSkillSchematicId04 = file.getUint(recordIndex, col++);
            tradeSkillSchematicId05 = file.getUint(recordIndex, col++);
            tradeSkillSchematicId06 = file.getUint(recordIndex, col++);
            tradeSkillSchematicId07 = file.getUint(recordIndex, col++);
        }
    };
}