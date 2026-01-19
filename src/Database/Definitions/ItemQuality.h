#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct ItemQuality
    {
        uint32_t ID;
        float salvageCritChance;
        float turninMultiplier;
        float runeCostMultiplier;
        float dyeCostMultiplier;
        uint32_t visualEffectIdLoot;
        float iconColorR;
        float iconColorG;
        float iconColorB;
        uint32_t defaultRunes;
        uint32_t maxRunes;
        std::wstring assetPathDieModel;
        uint32_t soundEventIdFortuneCardFanfare;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            salvageCritChance = file.getFloat(recordIndex, i++);
            turninMultiplier = file.getFloat(recordIndex, i++);
            runeCostMultiplier = file.getFloat(recordIndex, i++);
            dyeCostMultiplier = file.getFloat(recordIndex, i++);
            visualEffectIdLoot = file.getUint(recordIndex, i++);
            iconColorR = file.getFloat(recordIndex, i++);
            iconColorG = file.getFloat(recordIndex, i++);
            iconColorB = file.getFloat(recordIndex, i++);
            defaultRunes = file.getUint(recordIndex, i++);
            maxRunes = file.getUint(recordIndex, i++);
            assetPathDieModel = file.getString(recordIndex, i++);
            soundEventIdFortuneCardFanfare = file.getUint(recordIndex, i++);
        }
    };
}
