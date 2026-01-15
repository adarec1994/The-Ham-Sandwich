#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct ResourceConversionGroup
    {
        uint32_t ID;
        uint32_t flags;
        uint32_t resourceConversionId00;
        uint32_t resourceConversionId01;
        uint32_t resourceConversionId02;
        uint32_t resourceConversionId03;
        uint32_t resourceConversionId04;
        uint32_t resourceConversionId05;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            flags = f.getUint(row, i++);
            resourceConversionId00 = f.getUint(row, i++);
            resourceConversionId01 = f.getUint(row, i++);
            resourceConversionId02 = f.getUint(row, i++);
            resourceConversionId03 = f.getUint(row, i++);
            resourceConversionId04 = f.getUint(row, i++);
            resourceConversionId05 = f.getUint(row, i++);
        }
    };
}
