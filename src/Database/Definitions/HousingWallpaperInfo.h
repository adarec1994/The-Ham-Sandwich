#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct HousingWallpaperInfo
    {
        uint32_t ID;
        uint32_t localizedTextId;
        uint32_t cost;
        uint32_t costCurrencyTypeId;
        uint32_t replaceableMaterialInfoId;
        uint32_t worldSkyId;
        uint32_t flags;
        uint32_t prerequisiteIdUnlock;
        uint32_t prerequisiteIdUse;
        uint32_t unlockIndex;
        uint32_t soundZoneKitId;
        uint32_t worldLayerId00;
        uint32_t worldLayerId01;
        uint32_t worldLayerId02;
        uint32_t worldLayerId03;
        uint32_t accountItemIdUpsell;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextId = file.getUint(recordIndex, i++);
            cost = file.getUint(recordIndex, i++);
            costCurrencyTypeId = file.getUint(recordIndex, i++);
            replaceableMaterialInfoId = file.getUint(recordIndex, i++);
            worldSkyId = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            prerequisiteIdUnlock = file.getUint(recordIndex, i++);
            prerequisiteIdUse = file.getUint(recordIndex, i++);
            unlockIndex = file.getUint(recordIndex, i++);
            soundZoneKitId = file.getUint(recordIndex, i++);
            worldLayerId00 = file.getUint(recordIndex, i++);
            worldLayerId01 = file.getUint(recordIndex, i++);
            worldLayerId02 = file.getUint(recordIndex, i++);
            worldLayerId03 = file.getUint(recordIndex, i++);
            accountItemIdUpsell = file.getUint(recordIndex, i++);
        }
    };
}
