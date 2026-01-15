#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct RandomTextLineSet
    {
        uint32_t ID;
        uint32_t generalVOSetEnumFirst00;
        uint32_t generalVOSetEnumFirst01;
        uint32_t generalVOSetEnumFirst02;
        uint32_t generalVOSetEnumFirst03;
        uint32_t generalVOSetEnumFirst04;
        uint32_t generalVOSetEnumFirst05;
        uint32_t generalVOSetEnumFirst06;
        uint32_t generalVOSetEnumFirst07;
        uint32_t generalVOSetEnumFirst08;
        uint32_t generalVOSetEnumFirst09;
        uint32_t generalVOSetEnumFirst10;
        uint32_t generalVOSetEnumFirst11;
        uint32_t generalVOSetEnumFirst12;
        uint32_t generalVOSetEnumFirst13;
        uint32_t generalVOSetEnumFirst14;
        uint32_t generalVOSetEnumSecond00;
        uint32_t generalVOSetEnumSecond01;
        uint32_t generalVOSetEnumSecond02;
        uint32_t generalVOSetEnumSecond03;
        uint32_t generalVOSetEnumSecond04;
        uint32_t generalVOSetEnumSecond05;
        uint32_t generalVOSetEnumSecond06;
        uint32_t generalVOSetEnumSecond07;
        uint32_t generalVOSetEnumSecond08;
        uint32_t generalVOSetEnumSecond09;
        uint32_t generalVOSetEnumSecond10;
        uint32_t generalVOSetEnumSecond11;
        uint32_t generalVOSetEnumSecond12;
        uint32_t generalVOSetEnumSecond13;
        uint32_t generalVOSetEnumSecond14;
        uint32_t localizedTextId00;
        uint32_t localizedTextId01;
        uint32_t localizedTextId02;
        uint32_t localizedTextId03;
        uint32_t localizedTextId04;
        uint32_t localizedTextId05;
        uint32_t localizedTextId06;
        uint32_t localizedTextId07;
        uint32_t localizedTextId08;
        uint32_t localizedTextId09;
        uint32_t localizedTextId10;
        uint32_t localizedTextId11;
        uint32_t localizedTextId12;
        uint32_t localizedTextId13;
        uint32_t localizedTextId14;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            generalVOSetEnumFirst00 = f.getUint(row, i++);
            generalVOSetEnumFirst01 = f.getUint(row, i++);
            generalVOSetEnumFirst02 = f.getUint(row, i++);
            generalVOSetEnumFirst03 = f.getUint(row, i++);
            generalVOSetEnumFirst04 = f.getUint(row, i++);
            generalVOSetEnumFirst05 = f.getUint(row, i++);
            generalVOSetEnumFirst06 = f.getUint(row, i++);
            generalVOSetEnumFirst07 = f.getUint(row, i++);
            generalVOSetEnumFirst08 = f.getUint(row, i++);
            generalVOSetEnumFirst09 = f.getUint(row, i++);
            generalVOSetEnumFirst10 = f.getUint(row, i++);
            generalVOSetEnumFirst11 = f.getUint(row, i++);
            generalVOSetEnumFirst12 = f.getUint(row, i++);
            generalVOSetEnumFirst13 = f.getUint(row, i++);
            generalVOSetEnumFirst14 = f.getUint(row, i++);
            generalVOSetEnumSecond00 = f.getUint(row, i++);
            generalVOSetEnumSecond01 = f.getUint(row, i++);
            generalVOSetEnumSecond02 = f.getUint(row, i++);
            generalVOSetEnumSecond03 = f.getUint(row, i++);
            generalVOSetEnumSecond04 = f.getUint(row, i++);
            generalVOSetEnumSecond05 = f.getUint(row, i++);
            generalVOSetEnumSecond06 = f.getUint(row, i++);
            generalVOSetEnumSecond07 = f.getUint(row, i++);
            generalVOSetEnumSecond08 = f.getUint(row, i++);
            generalVOSetEnumSecond09 = f.getUint(row, i++);
            generalVOSetEnumSecond10 = f.getUint(row, i++);
            generalVOSetEnumSecond11 = f.getUint(row, i++);
            generalVOSetEnumSecond12 = f.getUint(row, i++);
            generalVOSetEnumSecond13 = f.getUint(row, i++);
            generalVOSetEnumSecond14 = f.getUint(row, i++);
            localizedTextId00 = f.getUint(row, i++);
            localizedTextId01 = f.getUint(row, i++);
            localizedTextId02 = f.getUint(row, i++);
            localizedTextId03 = f.getUint(row, i++);
            localizedTextId04 = f.getUint(row, i++);
            localizedTextId05 = f.getUint(row, i++);
            localizedTextId06 = f.getUint(row, i++);
            localizedTextId07 = f.getUint(row, i++);
            localizedTextId08 = f.getUint(row, i++);
            localizedTextId09 = f.getUint(row, i++);
            localizedTextId10 = f.getUint(row, i++);
            localizedTextId11 = f.getUint(row, i++);
            localizedTextId12 = f.getUint(row, i++);
            localizedTextId13 = f.getUint(row, i++);
            localizedTextId14 = f.getUint(row, i++);
        }
    };
}
