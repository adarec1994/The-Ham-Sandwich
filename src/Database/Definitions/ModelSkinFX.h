#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct ModelSkinFX
    {
        uint32_t ID;
        std::wstring suffix;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            suffix = file.getString(recordIndex, i++);
        }
    };
}
