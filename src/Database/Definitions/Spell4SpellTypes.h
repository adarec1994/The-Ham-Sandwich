#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct Spell4SpellTypes
    {
        static constexpr const char* GetFileName() { return "Spell4SpellTypes"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        std::wstring typeName;
        std::wstring enumName;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            typeName = file.getString(recordIndex, col++);
            enumName = file.getString(recordIndex, col++);
        }
    };
}