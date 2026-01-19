#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct CustomizationParameter
    {
        uint32_t ID;
        uint32_t localizedTextId;
        uint32_t sclX;
        uint32_t sclY;
        uint32_t sclZ;
        uint32_t rotX;
        uint32_t rotY;
        uint32_t rotZ;
        uint32_t posX;
        uint32_t posY;
        uint32_t posZ;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextId = file.getUint(recordIndex, i++);
            sclX = file.getUint(recordIndex, i++);
            sclY = file.getUint(recordIndex, i++);
            sclZ = file.getUint(recordIndex, i++);
            rotX = file.getUint(recordIndex, i++);
            rotY = file.getUint(recordIndex, i++);
            rotZ = file.getUint(recordIndex, i++);
            posX = file.getUint(recordIndex, i++);
            posY = file.getUint(recordIndex, i++);
            posZ = file.getUint(recordIndex, i++);
        }
    };
}
