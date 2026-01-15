#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Race
    {
        uint32_t ID;
        std::wstring enumName;
        uint32_t localizedTextId;
        uint32_t localizedTextIdNameFemale;
        std::wstring maleAssetPath;
        std::wstring femaleAssetPath;
        float hitRadius;
        uint32_t soundImpactDescriptionIdOrigin;
        uint32_t soundImpactDescriptionIdTarget;
        float walkLand;
        float walkAir;
        float walkWater;
        float walkHover;
        uint32_t unitVisualTypeIdMale;
        uint32_t unitVisualTypeIdFemale;
        uint32_t soundEventIdMaleHealthStart;
        uint32_t soundEventIdFemaleHealthStart;
        uint32_t soundEventIdMaleHealthStop;
        uint32_t soundEventIdFemaleHealthStop;
        float swimWaterDepth;
        uint32_t itemDisplayIdUnderwearLegsMale;
        uint32_t itemDisplayIdUnderwearLegsFemale;
        uint32_t itemDisplayIdUnderwearChestFemale;
        uint32_t itemDisplayIdArmCannon;
        float mountScaleMale;
        float mountScaleFemale;
        uint32_t soundSwitchId;
        uint32_t componentLayoutIdMale;
        uint32_t componentLayoutIdFemale;
        uint32_t modelMeshIdMountItemMale;
        uint32_t modelMeshIdMountItemFemale;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            enumName = f.getString(row, i++);
            localizedTextId = f.getUint(row, i++);
            localizedTextIdNameFemale = f.getUint(row, i++);
            maleAssetPath = f.getString(row, i++);
            femaleAssetPath = f.getString(row, i++);
            hitRadius = f.getFloat(row, i++);
            soundImpactDescriptionIdOrigin = f.getUint(row, i++);
            soundImpactDescriptionIdTarget = f.getUint(row, i++);
            walkLand = f.getFloat(row, i++);
            walkAir = f.getFloat(row, i++);
            walkWater = f.getFloat(row, i++);
            walkHover = f.getFloat(row, i++);
            unitVisualTypeIdMale = f.getUint(row, i++);
            unitVisualTypeIdFemale = f.getUint(row, i++);
            soundEventIdMaleHealthStart = f.getUint(row, i++);
            soundEventIdFemaleHealthStart = f.getUint(row, i++);
            soundEventIdMaleHealthStop = f.getUint(row, i++);
            soundEventIdFemaleHealthStop = f.getUint(row, i++);
            swimWaterDepth = f.getFloat(row, i++);
            itemDisplayIdUnderwearLegsMale = f.getUint(row, i++);
            itemDisplayIdUnderwearLegsFemale = f.getUint(row, i++);
            itemDisplayIdUnderwearChestFemale = f.getUint(row, i++);
            itemDisplayIdArmCannon = f.getUint(row, i++);
            mountScaleMale = f.getFloat(row, i++);
            mountScaleFemale = f.getFloat(row, i++);
            soundSwitchId = f.getUint(row, i++);
            componentLayoutIdMale = f.getUint(row, i++);
            componentLayoutIdFemale = f.getUint(row, i++);
            modelMeshIdMountItemMale = f.getUint(row, i++);
            modelMeshIdMountItemFemale = f.getUint(row, i++);
        }
    };
}
