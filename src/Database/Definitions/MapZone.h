#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct MapZone
    {
        uint32_t ID;
        uint32_t localizedTextIdName;
        uint32_t mapContinentId;
        std::wstring folder;
        uint32_t hexMinX;
        uint32_t hexMinY;
        uint32_t hexLimX;
        uint32_t hexLimY;
        uint32_t version;
        uint32_t mapZoneIdParent;
        uint32_t worldZoneId;
        uint32_t flags;
        uint32_t prerequisiteIdVisibility;
        uint32_t rewardTrackId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            mapContinentId = file.getUint(recordIndex, i++);
            folder = file.getString(recordIndex, i++);
            hexMinX = file.getUint(recordIndex, i++);
            hexMinY = file.getUint(recordIndex, i++);
            hexLimX = file.getUint(recordIndex, i++);
            hexLimY = file.getUint(recordIndex, i++);
            version = file.getUint(recordIndex, i++);
            mapZoneIdParent = file.getUint(recordIndex, i++);
            worldZoneId = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            prerequisiteIdVisibility = file.getUint(recordIndex, i++);
            rewardTrackId = file.getUint(recordIndex, i++);
        }
    };
}
