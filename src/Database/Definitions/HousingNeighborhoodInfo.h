#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct HousingNeighborhoodInfo
    {
        uint32_t ID;
        uint32_t baseCost;
        uint32_t maxPopulation;
        uint32_t populationThreshold;
        uint32_t housingFactionEnum;
        uint32_t housingFeatureTypeEnum;
        uint32_t housingPlaystyleTypeEnum;
        uint32_t housingMapInfoIdPrimary;
        uint32_t housingMapInfoIdSecondary;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            baseCost = file.getUint(recordIndex, i++);
            maxPopulation = file.getUint(recordIndex, i++);
            populationThreshold = file.getUint(recordIndex, i++);
            housingFactionEnum = file.getUint(recordIndex, i++);
            housingFeatureTypeEnum = file.getUint(recordIndex, i++);
            housingPlaystyleTypeEnum = file.getUint(recordIndex, i++);
            housingMapInfoIdPrimary = file.getUint(recordIndex, i++);
            housingMapInfoIdSecondary = file.getUint(recordIndex, i++);
        }
    };
}
