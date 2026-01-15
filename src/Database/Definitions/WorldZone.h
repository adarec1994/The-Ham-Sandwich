#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct WorldZone
    {
        static constexpr const char* GetFileName() { return "WorldZone"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t localizedTextIdName;
        uint32_t parentZoneId;
        uint32_t allowAccess;
        uint32_t color;
        uint32_t soundZoneKitId;
        uint32_t worldLocation2IdExit;
        uint32_t flags;
        uint32_t zonePvpRulesEnum;
        uint32_t rewardRotationContentId;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            localizedTextIdName = file.getUint(recordIndex, col++);
            parentZoneId = file.getUint(recordIndex, col++);
            allowAccess = file.getUint(recordIndex, col++);
            color = file.getUint(recordIndex, col++);
            soundZoneKitId = file.getUint(recordIndex, col++);
            worldLocation2IdExit = file.getUint(recordIndex, col++);
            flags = file.getUint(recordIndex, col++);
            zonePvpRulesEnum = file.getUint(recordIndex, col++);
            rewardRotationContentId = file.getUint(recordIndex, col++);
        }
    };
}