#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct HousingMannequinPose
    {
        uint32_t ID;
        std::wstring enumName;
        uint32_t localizedTextId;
        uint32_t modelSequenceId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            enumName = file.getString(recordIndex, i++);
            localizedTextId = file.getUint(recordIndex, i++);
            modelSequenceId = file.getUint(recordIndex, i++);
        }
    };
}
