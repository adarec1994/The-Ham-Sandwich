#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct PathSettlerImprovement
    {
        uint32_t ID;
        uint32_t countResource00;
        uint32_t countResource01;
        uint32_t countResource02;
        uint32_t countRecontributionResource00;
        uint32_t countRecontributionResource01;
        uint32_t countRecontributionResource02;
        uint32_t spell4IdDisplay;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            countResource00 = file.getUint(recordIndex, i++);
            countResource01 = file.getUint(recordIndex, i++);
            countResource02 = file.getUint(recordIndex, i++);
            countRecontributionResource00 = file.getUint(recordIndex, i++);
            countRecontributionResource01 = file.getUint(recordIndex, i++);
            countRecontributionResource02 = file.getUint(recordIndex, i++);
            spell4IdDisplay = file.getUint(recordIndex, i++);
        }
    };
}
