#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct Spell4Modification
    {
        static constexpr const char* GetFileName() { return "Spell4Modification"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t spell4EffectsId;
        uint32_t modificationParameterEnum;
        uint32_t priority;
        uint32_t modificationTypeEnum;
        float data00;
        float data01;
        float data02;
        float data03;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            spell4EffectsId = file.getUint(recordIndex, col++);
            modificationParameterEnum = file.getUint(recordIndex, col++);
            priority = file.getUint(recordIndex, col++);
            modificationTypeEnum = file.getUint(recordIndex, col++);
            data00 = file.getFloat(recordIndex, col++);
            data01 = file.getFloat(recordIndex, col++);
            data02 = file.getFloat(recordIndex, col++);
            data03 = file.getFloat(recordIndex, col++);
        }
    };
}