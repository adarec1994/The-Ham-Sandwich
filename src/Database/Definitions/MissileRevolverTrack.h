#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct MissileRevolverTrack
    {
        uint32_t ID;
        float radius;
        float speed;
        float speedMultiplier;
        float scaleMultiplier;
        uint32_t modelAttachmentIdHeight;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            radius = file.getFloat(recordIndex, i++);
            speed = file.getFloat(recordIndex, i++);
            speedMultiplier = file.getFloat(recordIndex, i++);
            scaleMultiplier = file.getFloat(recordIndex, i++);
            modelAttachmentIdHeight = file.getUint(recordIndex, i++);
        }
    };
}
