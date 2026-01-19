#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct Creature2ModelInfo
    {
        uint32_t ID;
        std::wstring assetPath;
        std::wstring assetTexture0;
        std::wstring assetTexture1;
        uint32_t modelTextureId0;
        uint32_t modelTextureId1;
        uint32_t creatureMaterialEnum;
        float scale;
        float hitRadius;
        float walkLand;
        float walkAir;
        float walkWater;
        float walkHover;
        float runLand;
        float runAir;
        float runWater;
        float runHover;
        uint32_t itemVisualTypeIdFeet;
        float swimWaterDepth;
        uint32_t race;
        uint32_t sex;
        uint32_t itemDisplayId[20];
        uint32_t modelMeshId[16];
        float groundOffsetHover;
        float groundOffsetFly;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            assetPath = file.getString(recordIndex, i++);
            assetTexture0 = file.getString(recordIndex, i++);
            assetTexture1 = file.getString(recordIndex, i++);
            modelTextureId0 = file.getUint(recordIndex, i++);
            modelTextureId1 = file.getUint(recordIndex, i++);
            creatureMaterialEnum = file.getUint(recordIndex, i++);
            scale = file.getFloat(recordIndex, i++);
            hitRadius = file.getFloat(recordIndex, i++);
            walkLand = file.getFloat(recordIndex, i++);
            walkAir = file.getFloat(recordIndex, i++);
            walkWater = file.getFloat(recordIndex, i++);
            walkHover = file.getFloat(recordIndex, i++);
            runLand = file.getFloat(recordIndex, i++);
            runAir = file.getFloat(recordIndex, i++);
            runWater = file.getFloat(recordIndex, i++);
            runHover = file.getFloat(recordIndex, i++);
            itemVisualTypeIdFeet = file.getUint(recordIndex, i++);
            swimWaterDepth = file.getFloat(recordIndex, i++);
            race = file.getUint(recordIndex, i++);
            sex = file.getUint(recordIndex, i++);
            for (int j = 0; j < 20; ++j)
                itemDisplayId[j] = file.getUint(recordIndex, i++);
            for (int j = 0; j < 16; ++j)
                modelMeshId[j] = file.getUint(recordIndex, i++);
            groundOffsetHover = file.getFloat(recordIndex, i++);
            groundOffsetFly = file.getFloat(recordIndex, i++);
        }
    };
}
