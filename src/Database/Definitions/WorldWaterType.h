#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct WorldWaterType
    {
        static constexpr const char* GetFileName() { return "WorldWaterType"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t worldWaterFogId;
        uint32_t SurfaceType;
        std::wstring particleFile;
        uint32_t soundDirectionalAmbienceId;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            worldWaterFogId = file.getUint(recordIndex, col++);
            SurfaceType = file.getUint(recordIndex, col++);
            particleFile = file.getString(recordIndex, col++);
            soundDirectionalAmbienceId = file.getUint(recordIndex, col++);
        }
    };
}