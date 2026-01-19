#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct HousingWarplotBossToken
    {
        uint32_t ID;
        uint32_t spell4IdSummon;
        uint32_t minimumUpgradeTierEnum;
        uint32_t housingPlugItemIdLinked;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            spell4IdSummon = file.getUint(recordIndex, i++);
            minimumUpgradeTierEnum = file.getUint(recordIndex, i++);
            housingPlugItemIdLinked = file.getUint(recordIndex, i++);
        }
    };
}
