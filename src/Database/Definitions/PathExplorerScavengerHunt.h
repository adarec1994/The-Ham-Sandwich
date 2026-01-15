#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct PathExplorerScavengerHunt
    {
        uint32_t ID;
        uint32_t creature2IdStart;
        uint32_t pathExplorerScavengerClueId00;
        uint32_t pathExplorerScavengerClueId01;
        uint32_t pathExplorerScavengerClueId02;
        uint32_t pathExplorerScavengerClueId03;
        uint32_t pathExplorerScavengerClueId04;
        uint32_t pathExplorerScavengerClueId05;
        uint32_t pathExplorerScavengerClueId06;
        uint32_t localizedTextIdStart;
        uint32_t spell4IdRelic;
        std::wstring assetPathSprite;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            creature2IdStart = file.getUint(recordIndex, i++);
            pathExplorerScavengerClueId00 = file.getUint(recordIndex, i++);
            pathExplorerScavengerClueId01 = file.getUint(recordIndex, i++);
            pathExplorerScavengerClueId02 = file.getUint(recordIndex, i++);
            pathExplorerScavengerClueId03 = file.getUint(recordIndex, i++);
            pathExplorerScavengerClueId04 = file.getUint(recordIndex, i++);
            pathExplorerScavengerClueId05 = file.getUint(recordIndex, i++);
            pathExplorerScavengerClueId06 = file.getUint(recordIndex, i++);
            localizedTextIdStart = file.getUint(recordIndex, i++);
            spell4IdRelic = file.getUint(recordIndex, i++);
            assetPathSprite = file.getString(recordIndex, i++);
        }
    };
}
