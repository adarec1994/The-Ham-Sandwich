#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct Wind
    {
        static constexpr const char* GetFileName() { return "Wind"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t type;
        uint32_t duration;
        float radiusEnd;
        float direction;
        float directionDelta;
        float blendIn;
        float blendOut;
        float speed;
        float sine2DMagnitudeMin;
        float sine2DMagnitudeMax;
        float sine2DFrequency;
        float sine2DOffsetAngle;
        uint32_t localRadial;
        float localMagnitude;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            type = file.getUint(recordIndex, col++);
            duration = file.getUint(recordIndex, col++);
            radiusEnd = file.getFloat(recordIndex, col++);
            direction = file.getFloat(recordIndex, col++);
            directionDelta = file.getFloat(recordIndex, col++);
            blendIn = file.getFloat(recordIndex, col++);
            blendOut = file.getFloat(recordIndex, col++);
            speed = file.getFloat(recordIndex, col++);
            sine2DMagnitudeMin = file.getFloat(recordIndex, col++);
            sine2DMagnitudeMax = file.getFloat(recordIndex, col++);
            sine2DFrequency = file.getFloat(recordIndex, col++);
            sine2DOffsetAngle = file.getFloat(recordIndex, col++);
            localRadial = file.getUint(recordIndex, col++);
            localMagnitude = file.getFloat(recordIndex, col++);
        }
    };
}