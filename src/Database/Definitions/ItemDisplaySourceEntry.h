#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct ItemDisplaySourceEntry
    {
        uint32_t ID;
        uint32_t itemSourceId;
        uint32_t item2TypeId;
        uint32_t itemMinLevel;
        uint32_t itemMaxLevel;
        uint32_t itemDisplayId;
        std::wstring icon;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            itemSourceId = file.getUint(recordIndex, i++);
            item2TypeId = file.getUint(recordIndex, i++);
            itemMinLevel = file.getUint(recordIndex, i++);
            itemMaxLevel = file.getUint(recordIndex, i++);
            itemDisplayId = file.getUint(recordIndex, i++);
            icon = file.getString(recordIndex, i++);
        }
    };
}
