#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct WorldWaterLayer
    {
        static constexpr const char* GetFileName() { return "WorldWaterLayer"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        std::wstring description;
        std::wstring RippleColorTex;
        std::wstring RippleNormalTex;
        float Scale;
        float Rotation;
        float Speed;
        float OscFrequency;
        float OscMagnitude;
        float OscRotation;
        float OscPhase;
        float OscMinLayerWeight;
        float OscMaxLayerWeight;
        float OscLayerWeightPhase;
        float materialBlend;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            description = file.getString(recordIndex, col++);
            RippleColorTex = file.getString(recordIndex, col++);
            RippleNormalTex = file.getString(recordIndex, col++);
            Scale = file.getFloat(recordIndex, col++);
            Rotation = file.getFloat(recordIndex, col++);
            Speed = file.getFloat(recordIndex, col++);
            OscFrequency = file.getFloat(recordIndex, col++);
            OscMagnitude = file.getFloat(recordIndex, col++);
            OscRotation = file.getFloat(recordIndex, col++);
            OscPhase = file.getFloat(recordIndex, col++);
            OscMinLayerWeight = file.getFloat(recordIndex, col++);
            OscMaxLayerWeight = file.getFloat(recordIndex, col++);
            OscLayerWeightPhase = file.getFloat(recordIndex, col++);
            materialBlend = file.getFloat(recordIndex, col++);
        }
    };
}