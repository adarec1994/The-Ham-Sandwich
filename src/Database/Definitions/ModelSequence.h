#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct ModelSequence
    {
        uint32_t ID;
        std::wstring description;
        uint32_t FallBackID;
        uint32_t flag;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            description = file.getString(recordIndex, i++);
            FallBackID = file.getUint(recordIndex, i++);
            flag = file.getUint(recordIndex, i++);
        }
    };
}
