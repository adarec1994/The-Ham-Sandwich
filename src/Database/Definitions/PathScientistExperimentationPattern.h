#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct PathScientistExperimentationPattern
    {
        uint32_t ID;
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdDescription;
        uint32_t pathMissionId;
        uint32_t pathScientistExperimentationId;
        std::wstring iconAssetPath;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            localizedTextIdDescription = file.getUint(recordIndex, i++);
            pathMissionId = file.getUint(recordIndex, i++);
            pathScientistExperimentationId = file.getUint(recordIndex, i++);
            iconAssetPath = file.getString(recordIndex, i++);
        }
    };
}
