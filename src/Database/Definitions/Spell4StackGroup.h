#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct Spell4StackGroup
    {
        static constexpr const char* GetFileName() { return "Spell4StackGroup"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t stackCap;
        uint32_t stackTypeEnum;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            stackCap = file.getUint(recordIndex, col++);
            stackTypeEnum = file.getUint(recordIndex, col++);
        }
    };
}