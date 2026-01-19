#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct CombatReward
    {
        uint32_t ID;
        uint32_t combatRewardTypeEnum;
        uint32_t localizedTextIdCombatFloater;
        uint32_t localizedTextIdCombatLogMessage;
        uint32_t visualEffectIdVisual[2];

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            combatRewardTypeEnum = file.getUint(recordIndex, i++);
            localizedTextIdCombatFloater = file.getUint(recordIndex, i++);
            localizedTextIdCombatLogMessage = file.getUint(recordIndex, i++);
            for (int j = 0; j < 2; ++j)
                visualEffectIdVisual[j] = file.getUint(recordIndex, i++);
        }
    };
}
