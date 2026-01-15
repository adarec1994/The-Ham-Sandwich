#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct QuestDirection
    {
        uint32_t ID;
        uint32_t questDirectionFlags;
        uint32_t questDirectionEntryId00;
        uint32_t questDirectionEntryId01;
        uint32_t questDirectionEntryId02;
        uint32_t questDirectionEntryId03;
        uint32_t questDirectionEntryId04;
        uint32_t questDirectionEntryId05;
        uint32_t questDirectionEntryId06;
        uint32_t questDirectionEntryId07;
        uint32_t questDirectionEntryId08;
        uint32_t questDirectionEntryId09;
        uint32_t questDirectionEntryId10;
        uint32_t questDirectionEntryId11;
        uint32_t questDirectionEntryId12;
        uint32_t questDirectionEntryId13;
        uint32_t questDirectionEntryId14;
        uint32_t questDirectionEntryId15;
        uint32_t worldZoneIdExcludedZone;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            questDirectionFlags = f.getUint(row, i++);
            questDirectionEntryId00 = f.getUint(row, i++);
            questDirectionEntryId01 = f.getUint(row, i++);
            questDirectionEntryId02 = f.getUint(row, i++);
            questDirectionEntryId03 = f.getUint(row, i++);
            questDirectionEntryId04 = f.getUint(row, i++);
            questDirectionEntryId05 = f.getUint(row, i++);
            questDirectionEntryId06 = f.getUint(row, i++);
            questDirectionEntryId07 = f.getUint(row, i++);
            questDirectionEntryId08 = f.getUint(row, i++);
            questDirectionEntryId09 = f.getUint(row, i++);
            questDirectionEntryId10 = f.getUint(row, i++);
            questDirectionEntryId11 = f.getUint(row, i++);
            questDirectionEntryId12 = f.getUint(row, i++);
            questDirectionEntryId13 = f.getUint(row, i++);
            questDirectionEntryId14 = f.getUint(row, i++);
            questDirectionEntryId15 = f.getUint(row, i++);
            worldZoneIdExcludedZone = f.getUint(row, i++);
        }
    };
}
