#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct ColorShift
    {
        uint32_t ID;
        std::wstring texturePath;
        uint32_t localizedTextId;
        std::wstring previewSwatchIcon;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            texturePath = file.getString(recordIndex, i++);
            localizedTextId = file.getUint(recordIndex, i++);
            previewSwatchIcon = file.getString(recordIndex, i++);
        }
    };
}
