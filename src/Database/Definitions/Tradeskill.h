#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct Tradeskill
    {
        static constexpr const char* GetFileName() { return "Tradeskill"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdDescription;
        uint32_t flags;
        uint32_t tutorialId;
        uint32_t achievementCategoryId;
        uint32_t maxAdditives;
        uint32_t localizedTextIdAxisName00;
        uint32_t localizedTextIdAxisName01;
        uint32_t localizedTextIdAxisName02;
        uint32_t localizedTextIdAxisName03;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            localizedTextIdName = file.getUint(recordIndex, col++);
            localizedTextIdDescription = file.getUint(recordIndex, col++);
            flags = file.getUint(recordIndex, col++);
            tutorialId = file.getUint(recordIndex, col++);
            achievementCategoryId = file.getUint(recordIndex, col++);
            maxAdditives = file.getUint(recordIndex, col++);
            localizedTextIdAxisName00 = file.getUint(recordIndex, col++);
            localizedTextIdAxisName01 = file.getUint(recordIndex, col++);
            localizedTextIdAxisName02 = file.getUint(recordIndex, col++);
            localizedTextIdAxisName03 = file.getUint(recordIndex, col++);
        }
    };
}