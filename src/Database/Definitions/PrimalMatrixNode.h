#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PrimalMatrixNode
    {
        uint32_t ID;
        uint32_t positionX;
        uint32_t positionY;
        uint32_t primalMatrixNodeTypeEnum;
        uint32_t flags;
        uint32_t maxAllocations;
        uint32_t costRedEssence;
        uint32_t costBlueEssence;
        uint32_t costGreenEssence;
        uint32_t costPurpleEssence;
        uint32_t primalMatrixRewardIdWarrior;
        uint32_t primalMatrixRewardIdEngineer;
        uint32_t primalMatrixRewardIdEsper;
        uint32_t primalMatrixRewardIdMedic;
        uint32_t primalMatrixRewardIdStalker;
        uint32_t primalMatrixRewardIdSpellslinger;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            positionX = f.getUint(row, i++);
            positionY = f.getUint(row, i++);
            primalMatrixNodeTypeEnum = f.getUint(row, i++);
            flags = f.getUint(row, i++);
            maxAllocations = f.getUint(row, i++);
            costRedEssence = f.getUint(row, i++);
            costBlueEssence = f.getUint(row, i++);
            costGreenEssence = f.getUint(row, i++);
            costPurpleEssence = f.getUint(row, i++);
            primalMatrixRewardIdWarrior = f.getUint(row, i++);
            primalMatrixRewardIdEngineer = f.getUint(row, i++);
            primalMatrixRewardIdEsper = f.getUint(row, i++);
            primalMatrixRewardIdMedic = f.getUint(row, i++);
            primalMatrixRewardIdStalker = f.getUint(row, i++);
            primalMatrixRewardIdSpellslinger = f.getUint(row, i++);
        }
    };
}
