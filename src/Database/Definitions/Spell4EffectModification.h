#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Spell4EffectModification
    {
        uint32_t ID;
        uint32_t spell4EffectsId;
        uint32_t effectTypeEnum;
        uint32_t modificationParameterEnum;
        uint32_t priority;
        uint32_t modificationTypeEnum;
        float data00;
        float data01;
        float data02;
        float data03;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            spell4EffectsId = f.getUint(row, i++);
            effectTypeEnum = f.getUint(row, i++);
            modificationParameterEnum = f.getUint(row, i++);
            priority = f.getUint(row, i++);
            modificationTypeEnum = f.getUint(row, i++);
            data00 = f.getFloat(row, i++);
            data01 = f.getFloat(row, i++);
            data02 = f.getFloat(row, i++);
            data03 = f.getFloat(row, i++);
        }
    };
}
