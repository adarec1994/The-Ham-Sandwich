#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PublicEventCustomStat
    {
        uint32_t ID;
        uint32_t publicEventTypeEnum;
        uint32_t publicEventId;
        uint32_t statIndex;
        uint32_t localizedTextIdStatName;
        std::wstring iconPath;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            publicEventTypeEnum = f.getUint(row, i++);
            publicEventId = f.getUint(row, i++);
            statIndex = f.getUint(row, i++);
            localizedTextIdStatName = f.getUint(row, i++);
            iconPath = f.getString(row, i++);
        }
    };
}
