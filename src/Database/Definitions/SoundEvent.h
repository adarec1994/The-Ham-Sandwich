#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct SoundEvent
    {
        uint32_t ID;
        std::wstring name;
        uint32_t hash;
        float radius;
        uint32_t soundBankId00;
        uint32_t soundBankId01;
        uint32_t soundBankId02;
        uint32_t soundBankId03;
        uint32_t soundBankId04;
        uint32_t soundBankId05;
        uint32_t soundBankId06;
        uint32_t soundBankId07;
        uint32_t flags;
        uint32_t limitPriority;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            name = f.getString(row, i++);
            hash = f.getUint(row, i++);
            radius = f.getFloat(row, i++);
            soundBankId00 = f.getUint(row, i++);
            soundBankId01 = f.getUint(row, i++);
            soundBankId02 = f.getUint(row, i++);
            soundBankId03 = f.getUint(row, i++);
            soundBankId04 = f.getUint(row, i++);
            soundBankId05 = f.getUint(row, i++);
            soundBankId06 = f.getUint(row, i++);
            soundBankId07 = f.getUint(row, i++);
            flags = f.getUint(row, i++);
            limitPriority = f.getUint(row, i++);
        }
    };
}
