#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TradeskillSchematic2
    {
        static constexpr const char* GetFileName() { return "TradeskillSchematic2"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t localizedTextIdName;
        uint32_t tradeSkillId;
        uint32_t item2IdOutput;
        uint32_t item2IdOutputFail;
        uint32_t outputCount;
        uint32_t lootId;
        uint32_t tier;
        uint32_t flags;
        uint32_t item2IdMaterial00;
        uint32_t item2IdMaterial01;
        uint32_t item2IdMaterial02;
        uint32_t item2IdMaterial03;
        uint32_t item2IdMaterial04;
        uint32_t materialCost00;
        uint32_t materialCost01;
        uint32_t materialCost02;
        uint32_t materialCost03;
        uint32_t materialCost04;
        uint32_t tradeskillSchematic2IdParent;
        float vectorX;
        float vectorY;
        float radius;
        float critRadius;
        uint32_t item2IdOutputCrit;
        uint32_t outputCountCritBonus;
        uint32_t priority;
        uint32_t maxAdditives;
        uint32_t discoverableQuadrant;
        float discoverableRadius;
        float discoverableAngle;
        uint32_t tradeskillCatalystOrderingId;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            localizedTextIdName = file.getUint(recordIndex, col++);
            tradeSkillId = file.getUint(recordIndex, col++);
            item2IdOutput = file.getUint(recordIndex, col++);
            item2IdOutputFail = file.getUint(recordIndex, col++);
            outputCount = file.getUint(recordIndex, col++);
            lootId = file.getUint(recordIndex, col++);
            tier = file.getUint(recordIndex, col++);
            flags = file.getUint(recordIndex, col++);
            item2IdMaterial00 = file.getUint(recordIndex, col++);
            item2IdMaterial01 = file.getUint(recordIndex, col++);
            item2IdMaterial02 = file.getUint(recordIndex, col++);
            item2IdMaterial03 = file.getUint(recordIndex, col++);
            item2IdMaterial04 = file.getUint(recordIndex, col++);
            materialCost00 = file.getUint(recordIndex, col++);
            materialCost01 = file.getUint(recordIndex, col++);
            materialCost02 = file.getUint(recordIndex, col++);
            materialCost03 = file.getUint(recordIndex, col++);
            materialCost04 = file.getUint(recordIndex, col++);
            tradeskillSchematic2IdParent = file.getUint(recordIndex, col++);
            vectorX = file.getFloat(recordIndex, col++);
            vectorY = file.getFloat(recordIndex, col++);
            radius = file.getFloat(recordIndex, col++);
            critRadius = file.getFloat(recordIndex, col++);
            item2IdOutputCrit = file.getUint(recordIndex, col++);
            outputCountCritBonus = file.getUint(recordIndex, col++);
            priority = file.getUint(recordIndex, col++);
            maxAdditives = file.getUint(recordIndex, col++);
            discoverableQuadrant = file.getUint(recordIndex, col++);
            discoverableRadius = file.getFloat(recordIndex, col++);
            discoverableAngle = file.getFloat(recordIndex, col++);
            tradeskillCatalystOrderingId = file.getUint(recordIndex, col++);
        }
    };
}