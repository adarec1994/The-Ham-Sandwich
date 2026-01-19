#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Creature2Difficulty
    {
        uint32_t ID;
        uint32_t localizedTextIdTitle;
        float pathScientistScanMultiplier;
        float clusterContributionValueDifficulty;
        uint32_t rankValue;
        uint32_t groupValue;
        float unitPropertyMultiplier[200];

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdTitle = file.getUint(recordIndex, i++);
            pathScientistScanMultiplier = file.getFloat(recordIndex, i++);
            clusterContributionValueDifficulty = file.getFloat(recordIndex, i++);
            rankValue = file.getUint(recordIndex, i++);
            groupValue = file.getUint(recordIndex, i++);
            for (int j = 0; j < 200; ++j)
                unitPropertyMultiplier[j] = file.getFloat(recordIndex, i++);
        }
    };
}
