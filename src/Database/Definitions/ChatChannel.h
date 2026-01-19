#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct ChatChannel
    {
        uint32_t ID;
        std::wstring enumName;
        std::wstring universalCommand[2];
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdCommand;
        uint32_t localizedTextIdAbbreviation;
        uint32_t localizedTextIdAlternate[2];
        uint32_t flags;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            enumName = file.getString(recordIndex, i++);
            for (int j = 0; j < 2; ++j)
                universalCommand[j] = file.getString(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            localizedTextIdCommand = file.getUint(recordIndex, i++);
            localizedTextIdAbbreviation = file.getUint(recordIndex, i++);
            for (int j = 0; j < 2; ++j)
                localizedTextIdAlternate[j] = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
        }
    };
}
