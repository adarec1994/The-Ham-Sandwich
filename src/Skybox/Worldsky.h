#pragma once
#include "../Tbl.h"
#include <string>
#include <cstdint>

namespace Database {
    namespace Definitions {

        struct WorldSky
        {
            uint32_t ID = 0;
            std::wstring AssetPath;

            void Read(const Tbl::File& file, uint32_t recordIndex)
            {
                ID = file.ReadUInt32(recordIndex, 0);
                AssetPath = file.ReadWString(recordIndex, 1);
            }

            uint32_t GetID() const { return ID; }
        };

    }
}