#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TaxiNode
    {
        static constexpr const char* GetFileName() { return "TaxiNode"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t localizedTextId;
        uint32_t taxiNodeTypeEnum;
        uint32_t flags;
        uint32_t flightPathTypeEnum;
        uint32_t taxiNodeFactionEnum;
        uint32_t worldLocation2Id;
        uint32_t contentTier;
        uint32_t autoUnlockLevel;
        uint32_t recommendedMinLevel;
        uint32_t recommendedMaxLevel;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            localizedTextId = file.getUint(recordIndex, col++);
            taxiNodeTypeEnum = file.getUint(recordIndex, col++);
            flags = file.getUint(recordIndex, col++);
            flightPathTypeEnum = file.getUint(recordIndex, col++);
            taxiNodeFactionEnum = file.getUint(recordIndex, col++);
            worldLocation2Id = file.getUint(recordIndex, col++);
            contentTier = file.getUint(recordIndex, col++);
            autoUnlockLevel = file.getUint(recordIndex, col++);
            recommendedMinLevel = file.getUint(recordIndex, col++);
            recommendedMaxLevel = file.getUint(recordIndex, col++);
        }
    };
}