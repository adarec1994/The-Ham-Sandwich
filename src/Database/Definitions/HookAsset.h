#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct HookAsset
    {
        uint32_t ID;
        std::wstring asset;
        float scale;
        float offsetX;
        float offsetY;
        float offsetZ;
        float rotationX;
        float rotationY;
        float rotationZ;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            asset = file.getString(recordIndex, i++);
            scale = file.getFloat(recordIndex, i++);
            offsetX = file.getFloat(recordIndex, i++);
            offsetY = file.getFloat(recordIndex, i++);
            offsetZ = file.getFloat(recordIndex, i++);
            rotationX = file.getFloat(recordIndex, i++);
            rotationY = file.getFloat(recordIndex, i++);
            rotationZ = file.getFloat(recordIndex, i++);
        }
    };
}
