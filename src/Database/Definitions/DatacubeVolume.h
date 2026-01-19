#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct DatacubeVolume
    {
        uint32_t ID;
        uint32_t localizedTextIdName;
        std::wstring assetPath;
        uint32_t datacubeId[16];

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            assetPath = file.getString(recordIndex, i++);
            for (int j = 0; j < 16; ++j)
                datacubeId[j] = file.getUint(recordIndex, i++);
        }
    };
}
