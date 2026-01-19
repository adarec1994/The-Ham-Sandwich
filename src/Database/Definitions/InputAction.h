#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct InputAction
    {
        uint32_t ID;
        std::wstring enumName;
        uint32_t localizedTextId;
        uint32_t inputActionCategoryId;
        uint32_t canHaveUpDownState;
        uint32_t displayIndex;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            enumName = file.getString(recordIndex, i++);
            localizedTextId = file.getUint(recordIndex, i++);
            inputActionCategoryId = file.getUint(recordIndex, i++);
            canHaveUpDownState = file.getUint(recordIndex, i++);
            displayIndex = file.getUint(recordIndex, i++);
        }
    };
}
