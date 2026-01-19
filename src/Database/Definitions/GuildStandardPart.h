#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct GuildStandardPart
    {
        uint32_t ID;
        uint32_t guildStandardPartTypeEnum;
        uint32_t localizedTextIdName;
        uint32_t itemDisplayIdStandard;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            guildStandardPartTypeEnum = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            itemDisplayIdStandard = file.getUint(recordIndex, i++);
        }
    };
}
