#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct WorldWaterFog
    {
        static constexpr const char* GetFileName() { return "WorldWaterFog"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        float fogStart;
        float fogEnd;
        float fogStartUW;
        float fogEndUW;
        float modStart;
        float modEnd;
        float modStartUW;
        float modEndUW;
        uint32_t skyColorIndex;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            fogStart = file.getFloat(recordIndex, col++);
            fogEnd = file.getFloat(recordIndex, col++);
            fogStartUW = file.getFloat(recordIndex, col++);
            fogEndUW = file.getFloat(recordIndex, col++);
            modStart = file.getFloat(recordIndex, col++);
            modEnd = file.getFloat(recordIndex, col++);
            modStartUW = file.getFloat(recordIndex, col++);
            modEndUW = file.getFloat(recordIndex, col++);
            skyColorIndex = file.getUint(recordIndex, col++);
        }
    };
}