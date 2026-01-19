#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct GuildPermission
    {
        uint32_t ID;
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdDescription;
        std::wstring luaVariable;
        uint32_t localizedTextIdCommand;
        uint32_t guildTypeEnumFlags;
        uint32_t displayIndex;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            localizedTextIdDescription = file.getUint(recordIndex, i++);
            luaVariable = file.getString(recordIndex, i++);
            localizedTextIdCommand = file.getUint(recordIndex, i++);
            guildTypeEnumFlags = file.getUint(recordIndex, i++);
            displayIndex = file.getUint(recordIndex, i++);
        }
    };
}
