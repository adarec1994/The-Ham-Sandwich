#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PublicEventVirtualItemDepot
    {
        uint32_t ID;
        uint32_t creature2Id;
        uint32_t virtualItemId00;
        uint32_t virtualItemId01;
        uint32_t virtualItemId02;
        uint32_t virtualItemId03;
        uint32_t virtualItemId04;
        uint32_t virtualItemId05;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            creature2Id = f.getUint(row, i++);
            virtualItemId00 = f.getUint(row, i++);
            virtualItemId01 = f.getUint(row, i++);
            virtualItemId02 = f.getUint(row, i++);
            virtualItemId03 = f.getUint(row, i++);
            virtualItemId04 = f.getUint(row, i++);
            virtualItemId05 = f.getUint(row, i++);
        }
    };
}
