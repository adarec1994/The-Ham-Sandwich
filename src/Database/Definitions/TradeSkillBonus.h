#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TradeskillBonus
    {
        static constexpr const char* GetFileName() { return "TradeskillBonus"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t tradeSkillTierId;
        uint32_t achievementId;
        std::wstring iconPath;
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdTooltip;
        uint32_t tradeskillBonusEnum00;
        uint32_t tradeskillBonusEnum01;
        uint32_t tradeskillBonusEnum02;
        uint32_t objectIdPrimary00;
        uint32_t objectIdPrimary01;
        uint32_t objectIdPrimary02;
        uint32_t objectIdSecondary00;
        uint32_t objectIdSecondary01;
        uint32_t objectIdSecondary02;
        uint32_t objectIdTertiary00;
        uint32_t objectIdTertiary01;
        uint32_t objectIdTertiary02;
        float value00;
        float value01;
        float value02;
        uint32_t valueInt00;
        uint32_t valueInt01;
        uint32_t valueInt02;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            tradeSkillTierId = file.getUint(recordIndex, col++);
            achievementId = file.getUint(recordIndex, col++);
            iconPath = file.getString(recordIndex, col++);
            localizedTextIdName = file.getUint(recordIndex, col++);
            localizedTextIdTooltip = file.getUint(recordIndex, col++);
            tradeskillBonusEnum00 = file.getUint(recordIndex, col++);
            tradeskillBonusEnum01 = file.getUint(recordIndex, col++);
            tradeskillBonusEnum02 = file.getUint(recordIndex, col++);
            objectIdPrimary00 = file.getUint(recordIndex, col++);
            objectIdPrimary01 = file.getUint(recordIndex, col++);
            objectIdPrimary02 = file.getUint(recordIndex, col++);
            objectIdSecondary00 = file.getUint(recordIndex, col++);
            objectIdSecondary01 = file.getUint(recordIndex, col++);
            objectIdSecondary02 = file.getUint(recordIndex, col++);
            objectIdTertiary00 = file.getUint(recordIndex, col++);
            objectIdTertiary01 = file.getUint(recordIndex, col++);
            objectIdTertiary02 = file.getUint(recordIndex, col++);
            value00 = file.getFloat(recordIndex, col++);
            value01 = file.getFloat(recordIndex, col++);
            value02 = file.getFloat(recordIndex, col++);
            valueInt00 = file.getUint(recordIndex, col++);
            valueInt01 = file.getUint(recordIndex, col++);
            valueInt02 = file.getUint(recordIndex, col++);
        }
    };
}