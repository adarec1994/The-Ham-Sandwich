#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TradeskillMaterial
    {
        static constexpr const char* GetFileName() { return "TradeskillMaterial"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t item2IdStatRevolution;
        uint32_t item2Id;
        uint32_t displayIndex;
        uint32_t tradeskillMaterialCategoryId;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            item2IdStatRevolution = file.getUint(recordIndex, col++);
            item2Id = file.getUint(recordIndex, col++);
            displayIndex = file.getUint(recordIndex, col++);
            tradeskillMaterialCategoryId = file.getUint(recordIndex, col++);
        }
    };
}