#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct GuildPerk
    {
        uint32_t ID;
        uint32_t localizedTextIdTitle;
        uint32_t localizedTextIdDescription;
        std::wstring luaSprite;
        std::wstring luaName;
        uint32_t purchaseInfluenceCost;
        uint32_t activateInfluenceCost;
        uint32_t spell4IdActivate;
        uint32_t guildPerkIdRequired00;
        uint32_t guildPerkIdRequired01;
        uint32_t guildPerkIdRequired02;
        uint32_t achievementIdRequired;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdTitle = file.getUint(recordIndex, i++);
            localizedTextIdDescription = file.getUint(recordIndex, i++);
            luaSprite = file.getString(recordIndex, i++);
            luaName = file.getString(recordIndex, i++);
            purchaseInfluenceCost = file.getUint(recordIndex, i++);
            activateInfluenceCost = file.getUint(recordIndex, i++);
            spell4IdActivate = file.getUint(recordIndex, i++);
            guildPerkIdRequired00 = file.getUint(recordIndex, i++);
            guildPerkIdRequired01 = file.getUint(recordIndex, i++);
            guildPerkIdRequired02 = file.getUint(recordIndex, i++);
            achievementIdRequired = file.getUint(recordIndex, i++);
        }
    };
}
