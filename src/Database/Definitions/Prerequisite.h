#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Prerequisite
    {
        uint32_t ID;
        uint32_t flags;
        uint32_t prerequisiteTypeId0;
        uint32_t prerequisiteTypeId1;
        uint32_t prerequisiteTypeId2;
        uint32_t prerequisiteComparisonId0;
        uint32_t prerequisiteComparisonId1;
        uint32_t prerequisiteComparisonId2;
        uint32_t objectId0;
        uint32_t objectId1;
        uint32_t objectId2;
        uint32_t value0;
        uint32_t value1;
        uint32_t value2;
        uint32_t localizedTextIdFailure;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            flags = f.getUint(row, i++);
            prerequisiteTypeId0 = f.getUint(row, i++);
            prerequisiteTypeId1 = f.getUint(row, i++);
            prerequisiteTypeId2 = f.getUint(row, i++);
            prerequisiteComparisonId0 = f.getUint(row, i++);
            prerequisiteComparisonId1 = f.getUint(row, i++);
            prerequisiteComparisonId2 = f.getUint(row, i++);
            objectId0 = f.getUint(row, i++);
            objectId1 = f.getUint(row, i++);
            objectId2 = f.getUint(row, i++);
            value0 = f.getUint(row, i++);
            value1 = f.getUint(row, i++);
            value2 = f.getUint(row, i++);
            localizedTextIdFailure = f.getUint(row, i++);
        }
    };
}
