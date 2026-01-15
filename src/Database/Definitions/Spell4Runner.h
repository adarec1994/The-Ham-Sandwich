#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct Spell4Runner
    {
        static constexpr const char* GetFileName() { return "Spell4Runner"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        std::wstring spriteName;
        float redTint;
        float greenTint;
        float blueTint;
        float alphaTint;
        float rate;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            spriteName = file.getString(recordIndex, col++);
            redTint = file.getFloat(recordIndex, col++);
            greenTint = file.getFloat(recordIndex, col++);
            blueTint = file.getFloat(recordIndex, col++);
            alphaTint = file.getFloat(recordIndex, col++);
            rate = file.getFloat(recordIndex, col++);
        }
    };
}