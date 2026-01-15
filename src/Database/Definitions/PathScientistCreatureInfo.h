#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct PathScientistCreatureInfo
    {
        uint32_t ID;
        uint32_t scientistCreatureFlags;
        std::wstring displayIcon;
        uint32_t prerequisiteIdScan;
        uint32_t prerequisiteIdRawScan;
        uint32_t prerequisiteIdScanCreature;
        uint32_t prerequisiteIdRawScanCreature;
        uint32_t spell4IdBuff00;
        uint32_t spell4IdBuff01;
        uint32_t spell4IdBuff02;
        uint32_t spell4IdBuff03;
        uint32_t checklistCount;
        uint32_t scientistCreatureTypeEnum;
        uint32_t lootId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            scientistCreatureFlags = file.getUint(recordIndex, i++);
            displayIcon = file.getString(recordIndex, i++);
            prerequisiteIdScan = file.getUint(recordIndex, i++);
            prerequisiteIdRawScan = file.getUint(recordIndex, i++);
            prerequisiteIdScanCreature = file.getUint(recordIndex, i++);
            prerequisiteIdRawScanCreature = file.getUint(recordIndex, i++);
            spell4IdBuff00 = file.getUint(recordIndex, i++);
            spell4IdBuff01 = file.getUint(recordIndex, i++);
            spell4IdBuff02 = file.getUint(recordIndex, i++);
            spell4IdBuff03 = file.getUint(recordIndex, i++);
            checklistCount = file.getUint(recordIndex, i++);
            scientistCreatureTypeEnum = file.getUint(recordIndex, i++);
            lootId = file.getUint(recordIndex, i++);
        }
    };
}
