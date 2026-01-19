#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct GenericMapNode
    {
        uint32_t ID;
        uint32_t genericMapId;
        uint32_t worldLocation2Id;
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdDescription;
        std::wstring spritePath;
        uint32_t genericMapNodeTypeEnum;
        uint32_t flags;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            genericMapId = file.getUint(recordIndex, i++);
            worldLocation2Id = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            localizedTextIdDescription = file.getUint(recordIndex, i++);
            spritePath = file.getString(recordIndex, i++);
            genericMapNodeTypeEnum = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
        }
    };
}
