#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct VirtualItem
    {
        static constexpr const char* GetFileName() { return "VirtualItem"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        std::wstring buttonIcon;
        uint32_t item2TypeId;
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdFlavor;
        uint32_t itemQualityId;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            buttonIcon = file.getString(recordIndex, col++);
            item2TypeId = file.getUint(recordIndex, col++);
            localizedTextIdName = file.getUint(recordIndex, col++);
            localizedTextIdFlavor = file.getUint(recordIndex, col++);
            itemQualityId = file.getUint(recordIndex, col++);
        }
    };
}