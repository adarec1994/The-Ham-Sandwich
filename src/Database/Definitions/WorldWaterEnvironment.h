#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct WorldWaterEnvironment
    {
        static constexpr const char* GetFileName() { return "WorldWaterEnvironment"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        std::wstring LandMapPath;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            LandMapPath = file.getString(recordIndex, col++);
        }
    };
}