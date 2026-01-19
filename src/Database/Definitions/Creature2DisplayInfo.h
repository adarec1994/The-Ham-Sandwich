#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct Creature2DisplayInfo
    {
        uint32_t ID;
        uint32_t statusFlags;
        std::wstring assetPath;
        std::wstring assetTexture[6];
        uint32_t modelTextureId[6];
        std::wstring colorShiftTexturePath;
        uint32_t colorShiftTextureIdMask;
        uint32_t materialDataIdSkinColor0;
        uint32_t materialDataIdSkinColor1;
        uint32_t materialDataIdWeaponColor0;
        uint32_t materialDataIdWeaponColor1;
        uint32_t materialDataIdArmorColor0;
        uint32_t materialDataIdArmorColor1;
        uint32_t materialDataIdDefault0;
        uint32_t materialDataIdDefault1;
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
        uint32_t raceId;
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
            statusFlags = file.getUint(recordIndex, i++);
            assetPath = file.getString(recordIndex, i++);
            for (int j = 0; j < 6; ++j)
                assetTexture[j] = file.getString(recordIndex, i++);
            for (int j = 0; j < 6; ++j)
                modelTextureId[j] = file.getUint(recordIndex, i++);
            colorShiftTexturePath = file.getString(recordIndex, i++);
            colorShiftTextureIdMask = file.getUint(recordIndex, i++);
            materialDataIdSkinColor0 = file.getUint(recordIndex, i++);
            materialDataIdSkinColor1 = file.getUint(recordIndex, i++);
            materialDataIdWeaponColor0 = file.getUint(recordIndex, i++);
            materialDataIdWeaponColor1 = file.getUint(recordIndex, i++);
            materialDataIdArmorColor0 = file.getUint(recordIndex, i++);
            materialDataIdArmorColor1 = file.getUint(recordIndex, i++);
            materialDataIdDefault0 = file.getUint(recordIndex, i++);
            materialDataIdDefault1 = file.getUint(recordIndex, i++);
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
            raceId = file.getUint(recordIndex, i++);
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
