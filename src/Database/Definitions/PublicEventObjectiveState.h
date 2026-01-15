#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PublicEventObjectiveState
    {
        uint32_t ID;
        uint32_t localizedTextIdState00;
        uint32_t localizedTextIdState01;
        uint32_t localizedTextIdState02;
        uint32_t localizedTextIdState03;
        uint32_t localizedTextIdState04;
        uint32_t localizedTextIdState05;
        uint32_t localizedTextIdState06;
        uint32_t localizedTextIdState07;
        uint32_t localizedTextIdState08;
        uint32_t localizedTextIdState09;
        uint32_t localizedTextIdState10;
        uint32_t localizedTextIdState11;
        uint32_t localizedTextIdState12;
        uint32_t localizedTextIdState13;
        uint32_t localizedTextIdState14;
        uint32_t localizedTextIdState15;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            localizedTextIdState00 = f.getUint(row, i++);
            localizedTextIdState01 = f.getUint(row, i++);
            localizedTextIdState02 = f.getUint(row, i++);
            localizedTextIdState03 = f.getUint(row, i++);
            localizedTextIdState04 = f.getUint(row, i++);
            localizedTextIdState05 = f.getUint(row, i++);
            localizedTextIdState06 = f.getUint(row, i++);
            localizedTextIdState07 = f.getUint(row, i++);
            localizedTextIdState08 = f.getUint(row, i++);
            localizedTextIdState09 = f.getUint(row, i++);
            localizedTextIdState10 = f.getUint(row, i++);
            localizedTextIdState11 = f.getUint(row, i++);
            localizedTextIdState12 = f.getUint(row, i++);
            localizedTextIdState13 = f.getUint(row, i++);
            localizedTextIdState14 = f.getUint(row, i++);
            localizedTextIdState15 = f.getUint(row, i++);
        }
    };
}
