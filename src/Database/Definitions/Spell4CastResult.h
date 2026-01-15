#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Spell4CastResult
    {
        uint32_t ID;
        std::wstring enumName;
        uint32_t combatMessageTypeEnum;
        uint32_t localizedTextIdDisplayText;
        uint32_t soundEventId;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            enumName = f.getString(row, i++);
            combatMessageTypeEnum = f.getUint(row, i++);
            localizedTextIdDisplayText = f.getUint(row, i++);
            soundEventId = f.getUint(row, i++);
        }
    };
}
