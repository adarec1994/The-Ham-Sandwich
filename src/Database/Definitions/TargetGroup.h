#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TargetGroup
    {
        static constexpr const char* GetFileName() { return "TargetGroup"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t localizedTextIdDisplayString;
        uint32_t type;
        uint32_t data0;
        uint32_t data1;
        uint32_t data2;
        uint32_t data3;
        uint32_t data4;
        uint32_t data5;
        uint32_t data6;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            localizedTextIdDisplayString = file.getUint(recordIndex, col++);
            type = file.getUint(recordIndex, col++);
            data0 = file.getUint(recordIndex, col++);
            data1 = file.getUint(recordIndex, col++);
            data2 = file.getUint(recordIndex, col++);
            data3 = file.getUint(recordIndex, col++);
            data4 = file.getUint(recordIndex, col++);
            data5 = file.getUint(recordIndex, col++);
            data6 = file.getUint(recordIndex, col++);
        }
    };
}