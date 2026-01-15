#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct ModelBonePriority
    {
        uint32_t ID;
        uint32_t BoneID;
        uint32_t BoneSetID;
        uint32_t Priority;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            BoneID = file.getUint(recordIndex, i++);
            BoneSetID = file.getUint(recordIndex, i++);
            Priority = file.getUint(recordIndex, i++);
        }
    };
}
