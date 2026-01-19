#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct PathExplorerScavengerHunt
    {
        uint32_t ID;
        uint32_t creature2IdStart;
        uint32_t pathExplorerScavengerClueId[7];
        uint32_t localizedTextIdStart;
        uint32_t spell4IdRelic;
        std::wstring assetPathSprite;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            creature2IdStart = file.getUint(recordIndex, i++);
            for (int j = 0; j < 7; ++j)
                pathExplorerScavengerClueId[j] = file.getUint(recordIndex, i++);
            localizedTextIdStart = file.getUint(recordIndex, i++);
            spell4IdRelic = file.getUint(recordIndex, i++);
            assetPathSprite = file.getString(recordIndex, i++);
        }
    };
}
