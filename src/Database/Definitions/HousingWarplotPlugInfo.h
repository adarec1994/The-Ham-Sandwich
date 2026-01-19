#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct HousingWarplotPlugInfo
    {
        uint32_t ID;
        uint32_t housingPlugItemId;
        uint32_t maintenanceCost;
        uint32_t upgradeCost00;
        uint32_t upgradeCost01;
        uint32_t upgradeCost02;
        uint32_t spell4IdAbility00;
        uint32_t spell4IdAbility01;
        uint32_t spell4IdAbility02;
        uint32_t spell4IdAbility03;
        uint32_t spell4IdAbility04;
        uint32_t spell4IdAbility05;
        uint32_t spell4IdAbility06;
        uint32_t spell4IdAbility07;
        uint32_t spell4IdAbility08;
        uint32_t spell4IdAbility09;
        uint32_t spell4IdAbility10;
        uint32_t spell4IdAbility11;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            housingPlugItemId = file.getUint(recordIndex, i++);
            maintenanceCost = file.getUint(recordIndex, i++);
            upgradeCost00 = file.getUint(recordIndex, i++);
            upgradeCost01 = file.getUint(recordIndex, i++);
            upgradeCost02 = file.getUint(recordIndex, i++);
            spell4IdAbility00 = file.getUint(recordIndex, i++);
            spell4IdAbility01 = file.getUint(recordIndex, i++);
            spell4IdAbility02 = file.getUint(recordIndex, i++);
            spell4IdAbility03 = file.getUint(recordIndex, i++);
            spell4IdAbility04 = file.getUint(recordIndex, i++);
            spell4IdAbility05 = file.getUint(recordIndex, i++);
            spell4IdAbility06 = file.getUint(recordIndex, i++);
            spell4IdAbility07 = file.getUint(recordIndex, i++);
            spell4IdAbility08 = file.getUint(recordIndex, i++);
            spell4IdAbility09 = file.getUint(recordIndex, i++);
            spell4IdAbility10 = file.getUint(recordIndex, i++);
            spell4IdAbility11 = file.getUint(recordIndex, i++);
        }
    };
}
