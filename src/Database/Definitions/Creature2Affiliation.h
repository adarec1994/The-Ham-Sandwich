#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct Creature2Affiliation
    {
        uint32_t ID;
        uint32_t localizedTextIdTitle;
        uint32_t miniMapMarkerId;
        std::wstring overheadIconAssetPath;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdTitle = file.getUint(recordIndex, i++);
            miniMapMarkerId = file.getUint(recordIndex, i++);
            overheadIconAssetPath = file.getString(recordIndex, i++);
        }
    };
}
