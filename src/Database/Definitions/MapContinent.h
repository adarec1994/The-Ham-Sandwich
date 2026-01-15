#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct MapContinent
    {
        uint32_t ID;
        uint32_t localizedTextIdName;
        std::wstring assetPath;
        std::wstring imagePath;
        uint32_t imageWidth;
        uint32_t imageHeight;
        uint32_t imageOffsetX;
        uint32_t imageOffsetY;
        uint32_t hexMinX;
        uint32_t hexMinY;
        uint32_t hexLimX;
        uint32_t hexLimY;
        uint32_t flags;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            assetPath = file.getString(recordIndex, i++);
            imagePath = file.getString(recordIndex, i++);
            imageWidth = file.getUint(recordIndex, i++);
            imageHeight = file.getUint(recordIndex, i++);
            imageOffsetX = file.getUint(recordIndex, i++);
            imageOffsetY = file.getUint(recordIndex, i++);
            hexMinX = file.getUint(recordIndex, i++);
            hexMinY = file.getUint(recordIndex, i++);
            hexLimX = file.getUint(recordIndex, i++);
            hexLimY = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
        }
    };
}
