#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct SpellEffectType
    {
        static constexpr const char* GetFileName() { return "SpellEffectType"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t flags;
        uint32_t dataType00;
        uint32_t dataType01;
        uint32_t dataType02;
        uint32_t dataType03;
        uint32_t dataType04;
        uint32_t dataType05;
        uint32_t dataType06;
        uint32_t dataType07;
        uint32_t dataType08;
        uint32_t dataType09;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            flags = file.getUint(recordIndex, col++);
            dataType00 = file.getUint(recordIndex, col++);
            dataType01 = file.getUint(recordIndex, col++);
            dataType02 = file.getUint(recordIndex, col++);
            dataType03 = file.getUint(recordIndex, col++);
            dataType04 = file.getUint(recordIndex, col++);
            dataType05 = file.getUint(recordIndex, col++);
            dataType06 = file.getUint(recordIndex, col++);
            dataType07 = file.getUint(recordIndex, col++);
            dataType08 = file.getUint(recordIndex, col++);
            dataType09 = file.getUint(recordIndex, col++);
        }
    };
}