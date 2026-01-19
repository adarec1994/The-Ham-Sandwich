#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct CustomizationParameterMap
    {
        uint32_t ID;
        uint32_t raceId;
        uint32_t genderEnum;
        uint32_t modelBoneId;
        uint32_t customizationParameterId;
        uint32_t dataOrder;
        uint32_t flags;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            raceId = file.getUint(recordIndex, i++);
            genderEnum = file.getUint(recordIndex, i++);
            modelBoneId = file.getUint(recordIndex, i++);
            customizationParameterId = file.getUint(recordIndex, i++);
            dataOrder = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
        }
    };
}
