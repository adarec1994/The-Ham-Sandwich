#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct WorldWaterWake
    {
        static constexpr const char* GetFileName() { return "WorldWaterWake"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t flags;
        std::wstring colorTexture;
        std::wstring normalTexture;
        std::wstring distortionTexture;
        uint32_t durationMin;
        uint32_t durationMax;
        float scaleStart;
        float scaleEnd;
        float alphaStart;
        float alphaEnd;
        float distortionWeight;
        float distortionScaleStart;
        float distortionScaleEnd;
        float distortionSpeedU;
        float distortionSpeedV;
        float positionOffsetX;
        float positionOffsetY;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            flags = file.getUint(recordIndex, col++);
            colorTexture = file.getString(recordIndex, col++);
            normalTexture = file.getString(recordIndex, col++);
            distortionTexture = file.getString(recordIndex, col++);
            durationMin = file.getUint(recordIndex, col++);
            durationMax = file.getUint(recordIndex, col++);
            scaleStart = file.getFloat(recordIndex, col++);
            scaleEnd = file.getFloat(recordIndex, col++);
            alphaStart = file.getFloat(recordIndex, col++);
            alphaEnd = file.getFloat(recordIndex, col++);
            distortionWeight = file.getFloat(recordIndex, col++);
            distortionScaleStart = file.getFloat(recordIndex, col++);
            distortionScaleEnd = file.getFloat(recordIndex, col++);
            distortionSpeedU = file.getFloat(recordIndex, col++);
            distortionSpeedV = file.getFloat(recordIndex, col++);
            positionOffsetX = file.getFloat(recordIndex, col++);
            positionOffsetY = file.getFloat(recordIndex, col++);
        }
    };
}