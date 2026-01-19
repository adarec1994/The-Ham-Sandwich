#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct HousingDecorInfo
    {
        uint32_t ID;
        uint32_t housingDecorTypeId;
        uint32_t hookTypeId;
        uint32_t localizedTextIdName;
        uint32_t flags;
        uint32_t hookAssetId;
        uint32_t cost;
        uint32_t costCurrencyTypeId;
        uint32_t creature2IdActiveProp;
        uint32_t prerequisiteIdUnlock;
        uint32_t spell4IdInteriorBuff;
        uint32_t housingDecorLimitCategoryId;
        std::wstring altPreviewAsset;
        std::wstring altEditAsset;
        float minScale;
        float maxScale;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            housingDecorTypeId = file.getUint(recordIndex, i++);
            hookTypeId = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            hookAssetId = file.getUint(recordIndex, i++);
            cost = file.getUint(recordIndex, i++);
            costCurrencyTypeId = file.getUint(recordIndex, i++);
            creature2IdActiveProp = file.getUint(recordIndex, i++);
            prerequisiteIdUnlock = file.getUint(recordIndex, i++);
            spell4IdInteriorBuff = file.getUint(recordIndex, i++);
            housingDecorLimitCategoryId = file.getUint(recordIndex, i++);
            altPreviewAsset = file.getString(recordIndex, i++);
            altEditAsset = file.getString(recordIndex, i++);
            minScale = file.getFloat(recordIndex, i++);
            maxScale = file.getFloat(recordIndex, i++);
        }
    };
}
