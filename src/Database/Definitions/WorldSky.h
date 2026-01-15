#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct WorldSky
    {
        static constexpr const char* GetFileName() { return "WorldSky"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        std::wstring assetPath;
        std::wstring assetPathInFlux;
        uint32_t color;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            assetPath = file.getString(recordIndex, col++);
            assetPathInFlux = file.getString(recordIndex, col++);
            color = file.getUint(recordIndex, col++);
        }
    };
}