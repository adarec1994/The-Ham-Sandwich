#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Spell4EffectGroupList
    {
        uint32_t ID;
        uint32_t spell4EffectGroupId00;
        uint32_t spell4EffectGroupId01;
        uint32_t spell4EffectGroupId02;
        uint32_t spell4EffectGroupId03;
        uint32_t spell4EffectGroupId04;
        uint32_t spell4EffectGroupId05;
        uint32_t spell4EffectGroupId06;
        uint32_t spell4EffectGroupId07;
        uint32_t spell4EffectGroupId08;
        uint32_t spell4EffectGroupId09;
        uint32_t spell4EffectGroupId10;
        uint32_t spell4EffectGroupId11;
        uint32_t spell4EffectGroupId12;
        uint32_t spell4EffectGroupId13;
        uint32_t spell4EffectGroupId14;
        uint32_t spell4EffectGroupId15;
        uint32_t spell4EffectGroupId16;
        uint32_t spell4EffectGroupId17;
        uint32_t spell4EffectGroupId18;
        uint32_t spell4EffectGroupId19;
        uint32_t spell4EffectGroupId20;
        uint32_t spell4EffectGroupId21;
        uint32_t spell4EffectGroupId22;
        uint32_t spell4EffectGroupId23;
        uint32_t spell4EffectGroupId24;
        uint32_t spell4EffectGroupId25;
        uint32_t spell4EffectGroupId26;
        uint32_t spell4EffectGroupId27;
        uint32_t spell4EffectGroupId28;
        uint32_t spell4EffectGroupId29;
        uint32_t spell4EffectGroupId30;
        uint32_t spell4EffectGroupId31;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            spell4EffectGroupId00 = f.getUint(row, i++);
            spell4EffectGroupId01 = f.getUint(row, i++);
            spell4EffectGroupId02 = f.getUint(row, i++);
            spell4EffectGroupId03 = f.getUint(row, i++);
            spell4EffectGroupId04 = f.getUint(row, i++);
            spell4EffectGroupId05 = f.getUint(row, i++);
            spell4EffectGroupId06 = f.getUint(row, i++);
            spell4EffectGroupId07 = f.getUint(row, i++);
            spell4EffectGroupId08 = f.getUint(row, i++);
            spell4EffectGroupId09 = f.getUint(row, i++);
            spell4EffectGroupId10 = f.getUint(row, i++);
            spell4EffectGroupId11 = f.getUint(row, i++);
            spell4EffectGroupId12 = f.getUint(row, i++);
            spell4EffectGroupId13 = f.getUint(row, i++);
            spell4EffectGroupId14 = f.getUint(row, i++);
            spell4EffectGroupId15 = f.getUint(row, i++);
            spell4EffectGroupId16 = f.getUint(row, i++);
            spell4EffectGroupId17 = f.getUint(row, i++);
            spell4EffectGroupId18 = f.getUint(row, i++);
            spell4EffectGroupId19 = f.getUint(row, i++);
            spell4EffectGroupId20 = f.getUint(row, i++);
            spell4EffectGroupId21 = f.getUint(row, i++);
            spell4EffectGroupId22 = f.getUint(row, i++);
            spell4EffectGroupId23 = f.getUint(row, i++);
            spell4EffectGroupId24 = f.getUint(row, i++);
            spell4EffectGroupId25 = f.getUint(row, i++);
            spell4EffectGroupId26 = f.getUint(row, i++);
            spell4EffectGroupId27 = f.getUint(row, i++);
            spell4EffectGroupId28 = f.getUint(row, i++);
            spell4EffectGroupId29 = f.getUint(row, i++);
            spell4EffectGroupId30 = f.getUint(row, i++);
            spell4EffectGroupId31 = f.getUint(row, i++);
        }
    };
}
