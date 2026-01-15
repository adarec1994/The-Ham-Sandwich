#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct WorldLayer
    {
        static constexpr const char* GetFileName() { return "WorldLayer"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        std::wstring Description;
        float HeightScale;
        float HeightOffset;
        float ParallaxScale;
        float ParallaxOffset;
        float MetersPerTextureTile;
        std::wstring ColorMapPath;
        std::wstring NormalMapPath;
        uint32_t AverageColor;
        uint32_t Projection;
        uint32_t materialType;
        uint32_t worldClutterId00;
        uint32_t worldClutterId01;
        uint32_t worldClutterId02;
        uint32_t worldClutterId03;
        float specularPower;
        uint32_t emissiveGlow;
        float scrollSpeed00;
        float scrollSpeed01;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            Description = file.getString(recordIndex, col++);
            HeightScale = file.getFloat(recordIndex, col++);
            HeightOffset = file.getFloat(recordIndex, col++);
            ParallaxScale = file.getFloat(recordIndex, col++);
            ParallaxOffset = file.getFloat(recordIndex, col++);
            MetersPerTextureTile = file.getFloat(recordIndex, col++);
            ColorMapPath = file.getString(recordIndex, col++);
            NormalMapPath = file.getString(recordIndex, col++);
            AverageColor = file.getUint(recordIndex, col++);
            Projection = file.getUint(recordIndex, col++);
            materialType = file.getUint(recordIndex, col++);
            worldClutterId00 = file.getUint(recordIndex, col++);
            worldClutterId01 = file.getUint(recordIndex, col++);
            worldClutterId02 = file.getUint(recordIndex, col++);
            worldClutterId03 = file.getUint(recordIndex, col++);
            specularPower = file.getFloat(recordIndex, col++);
            emissiveGlow = file.getUint(recordIndex, col++);
            scrollSpeed00 = file.getFloat(recordIndex, col++);
            scrollSpeed01 = file.getFloat(recordIndex, col++);
        }
    };
}