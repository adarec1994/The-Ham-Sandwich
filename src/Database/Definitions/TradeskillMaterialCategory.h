#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TradeskillMaterialCategory
    {
        static constexpr const char* GetFileName() { return "TradeskillMaterialCategory"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t localizedTextIdName;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            localizedTextIdName = file.getUint(recordIndex, col++);
        }
    };
}