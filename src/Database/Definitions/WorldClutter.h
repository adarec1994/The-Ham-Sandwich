#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct WorldClutter
    {
        static constexpr const char* GetFileName() { return "WorldClutter"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        std::wstring Description;
        float density;
        uint32_t clutterFlags;
        std::wstring assetPath0;
        std::wstring assetPath01;
        std::wstring assetPath02;
        std::wstring assetPath03;
        std::wstring assetPath04;
        std::wstring assetPath05;
        float assetWeight0;
        float assetWeight01;
        float assetWeight02;
        float assetWeight03;
        float assetWeight04;
        float assetWeight05;
        uint32_t flag0;
        uint32_t flag01;
        uint32_t flag02;
        uint32_t flag03;
        uint32_t flag04;
        uint32_t flag05;
        float minScale0;
        float minScale01;
        float minScale02;
        float minScale03;
        float minScale04;
        float minScale05;
        float rotationMin0;
        float rotationMin01;
        float rotationMin02;
        float rotationMin03;
        float rotationMin04;
        float rotationMin05;
        float rotationMax0;
        float rotationMax01;
        float rotationMax02;
        float rotationMax03;
        float rotationMax04;
        float rotationMax05;
        uint32_t emissiveGlow00;
        uint32_t emissiveGlow01;
        uint32_t emissiveGlow02;
        uint32_t emissiveGlow03;
        uint32_t emissiveGlow04;
        uint32_t emissiveGlow05;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            Description = file.getString(recordIndex, col++);
            density = file.getFloat(recordIndex, col++);
            clutterFlags = file.getUint(recordIndex, col++);
            assetPath0 = file.getString(recordIndex, col++);
            assetPath01 = file.getString(recordIndex, col++);
            assetPath02 = file.getString(recordIndex, col++);
            assetPath03 = file.getString(recordIndex, col++);
            assetPath04 = file.getString(recordIndex, col++);
            assetPath05 = file.getString(recordIndex, col++);
            assetWeight0 = file.getFloat(recordIndex, col++);
            assetWeight01 = file.getFloat(recordIndex, col++);
            assetWeight02 = file.getFloat(recordIndex, col++);
            assetWeight03 = file.getFloat(recordIndex, col++);
            assetWeight04 = file.getFloat(recordIndex, col++);
            assetWeight05 = file.getFloat(recordIndex, col++);
            flag0 = file.getUint(recordIndex, col++);
            flag01 = file.getUint(recordIndex, col++);
            flag02 = file.getUint(recordIndex, col++);
            flag03 = file.getUint(recordIndex, col++);
            flag04 = file.getUint(recordIndex, col++);
            flag05 = file.getUint(recordIndex, col++);
            minScale0 = file.getFloat(recordIndex, col++);
            minScale01 = file.getFloat(recordIndex, col++);
            minScale02 = file.getFloat(recordIndex, col++);
            minScale03 = file.getFloat(recordIndex, col++);
            minScale04 = file.getFloat(recordIndex, col++);
            minScale05 = file.getFloat(recordIndex, col++);
            rotationMin0 = file.getFloat(recordIndex, col++);
            rotationMin01 = file.getFloat(recordIndex, col++);
            rotationMin02 = file.getFloat(recordIndex, col++);
            rotationMin03 = file.getFloat(recordIndex, col++);
            rotationMin04 = file.getFloat(recordIndex, col++);
            rotationMin05 = file.getFloat(recordIndex, col++);
            rotationMax0 = file.getFloat(recordIndex, col++);
            rotationMax01 = file.getFloat(recordIndex, col++);
            rotationMax02 = file.getFloat(recordIndex, col++);
            rotationMax03 = file.getFloat(recordIndex, col++);
            rotationMax04 = file.getFloat(recordIndex, col++);
            rotationMax05 = file.getFloat(recordIndex, col++);
            emissiveGlow00 = file.getUint(recordIndex, col++);
            emissiveGlow01 = file.getUint(recordIndex, col++);
            emissiveGlow02 = file.getUint(recordIndex, col++);
            emissiveGlow03 = file.getUint(recordIndex, col++);
            emissiveGlow04 = file.getUint(recordIndex, col++);
            emissiveGlow05 = file.getUint(recordIndex, col++);
        }
    };
}