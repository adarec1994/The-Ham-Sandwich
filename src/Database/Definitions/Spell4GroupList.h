#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct Spell4GroupList
    {
        static constexpr const char* GetFileName() { return "Spell4GroupList"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t spellGroupId00;
        uint32_t spellGroupId01;
        uint32_t spellGroupId02;
        uint32_t spellGroupId03;
        uint32_t spellGroupId04;
        uint32_t spellGroupId05;
        uint32_t spellGroupId06;
        uint32_t spellGroupId07;
        uint32_t spellGroupId08;
        uint32_t spellGroupId09;
        uint32_t spellGroupId10;
        uint32_t spellGroupId11;
        uint32_t spellGroupId12;
        uint32_t spellGroupId13;
        uint32_t spellGroupId14;
        uint32_t spellGroupId15;
        uint32_t spellGroupId16;
        uint32_t spellGroupId17;
        uint32_t spellGroupId18;
        uint32_t spellGroupId19;
        uint32_t spellGroupId20;
        uint32_t spellGroupId21;
        uint32_t spellGroupId22;
        uint32_t spellGroupId23;
        uint32_t spellGroupId24;
        uint32_t spellGroupId25;
        uint32_t spellGroupId26;
        uint32_t spellGroupId27;
        uint32_t spellGroupId28;
        uint32_t spellGroupId29;
        uint32_t spellGroupId30;
        uint32_t spellGroupId31;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            spellGroupId00 = file.getUint(recordIndex, col++);
            spellGroupId01 = file.getUint(recordIndex, col++);
            spellGroupId02 = file.getUint(recordIndex, col++);
            spellGroupId03 = file.getUint(recordIndex, col++);
            spellGroupId04 = file.getUint(recordIndex, col++);
            spellGroupId05 = file.getUint(recordIndex, col++);
            spellGroupId06 = file.getUint(recordIndex, col++);
            spellGroupId07 = file.getUint(recordIndex, col++);
            spellGroupId08 = file.getUint(recordIndex, col++);
            spellGroupId09 = file.getUint(recordIndex, col++);
            spellGroupId10 = file.getUint(recordIndex, col++);
            spellGroupId11 = file.getUint(recordIndex, col++);
            spellGroupId12 = file.getUint(recordIndex, col++);
            spellGroupId13 = file.getUint(recordIndex, col++);
            spellGroupId14 = file.getUint(recordIndex, col++);
            spellGroupId15 = file.getUint(recordIndex, col++);
            spellGroupId16 = file.getUint(recordIndex, col++);
            spellGroupId17 = file.getUint(recordIndex, col++);
            spellGroupId18 = file.getUint(recordIndex, col++);
            spellGroupId19 = file.getUint(recordIndex, col++);
            spellGroupId20 = file.getUint(recordIndex, col++);
            spellGroupId21 = file.getUint(recordIndex, col++);
            spellGroupId22 = file.getUint(recordIndex, col++);
            spellGroupId23 = file.getUint(recordIndex, col++);
            spellGroupId24 = file.getUint(recordIndex, col++);
            spellGroupId25 = file.getUint(recordIndex, col++);
            spellGroupId26 = file.getUint(recordIndex, col++);
            spellGroupId27 = file.getUint(recordIndex, col++);
            spellGroupId28 = file.getUint(recordIndex, col++);
            spellGroupId29 = file.getUint(recordIndex, col++);
            spellGroupId30 = file.getUint(recordIndex, col++);
            spellGroupId31 = file.getUint(recordIndex, col++);
        }
    };
}