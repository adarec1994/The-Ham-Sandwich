#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct ModelPose
    {
        uint32_t ID;
        uint32_t sequenceId;
        std::wstring description;
        uint32_t modelPoseIdBase;
        uint32_t modelPoseTypeEnum;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            sequenceId = file.getUint(recordIndex, i++);
            description = file.getString(recordIndex, i++);
            modelPoseIdBase = file.getUint(recordIndex, i++);
            modelPoseTypeEnum = file.getUint(recordIndex, i++);
        }
    };
}
