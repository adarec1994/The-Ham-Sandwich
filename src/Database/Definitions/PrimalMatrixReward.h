#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PrimalMatrixReward
    {
        uint32_t ID;
        uint32_t primalMatrixRewardTypeEnum0;
        uint32_t primalMatrixRewardTypeEnum1;
        uint32_t primalMatrixRewardTypeEnum2;
        uint32_t primalMatrixRewardTypeEnum3;
        uint32_t objectId0;
        uint32_t objectId1;
        uint32_t objectId2;
        uint32_t objectId3;
        uint32_t subObjectId0;
        uint32_t subObjectId1;
        uint32_t subObjectId2;
        uint32_t subObjectId3;
        float value0;
        float value1;
        float value2;
        float value3;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            primalMatrixRewardTypeEnum0 = f.getUint(row, i++);
            primalMatrixRewardTypeEnum1 = f.getUint(row, i++);
            primalMatrixRewardTypeEnum2 = f.getUint(row, i++);
            primalMatrixRewardTypeEnum3 = f.getUint(row, i++);
            objectId0 = f.getUint(row, i++);
            objectId1 = f.getUint(row, i++);
            objectId2 = f.getUint(row, i++);
            objectId3 = f.getUint(row, i++);
            subObjectId0 = f.getUint(row, i++);
            subObjectId1 = f.getUint(row, i++);
            subObjectId2 = f.getUint(row, i++);
            subObjectId3 = f.getUint(row, i++);
            value0 = f.getFloat(row, i++);
            value1 = f.getFloat(row, i++);
            value2 = f.getFloat(row, i++);
            value3 = f.getFloat(row, i++);
        }
    };
}
