#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PathExplorerScavengerClue
    {
        uint32_t ID;
        uint32_t localizedTextIdClue;
        uint32_t explorerScavengerClueTypeEnum;
        uint32_t creature2Id;
        uint32_t targetGroupId;
        float activeRadius;
        uint32_t worldLocation2IdMiniMap;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdClue = file.getUint(recordIndex, i++);
            explorerScavengerClueTypeEnum = file.getUint(recordIndex, i++);
            creature2Id = file.getUint(recordIndex, i++);
            targetGroupId = file.getUint(recordIndex, i++);
            activeRadius = file.getFloat(recordIndex, i++);
            worldLocation2IdMiniMap = file.getUint(recordIndex, i++);
        }
    };
}
