#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct HousingResidenceInfo
    {
        uint32_t ID;
        uint32_t housingDecorInfoIdDefaultRoof;
        uint32_t housingDecorInfoIdDefaultEntryway;
        uint32_t housingDecorInfoIdDefaultDoor;
        uint32_t housingWallpaperInfoIdDefault;
        uint32_t worldLocation2IdInside00;
        uint32_t worldLocation2IdInside01;
        uint32_t worldLocation2IdInside02;
        uint32_t worldLocation2IdInside03;
        uint32_t worldLocation2IdOutside00;
        uint32_t worldLocation2IdOutside01;
        uint32_t worldLocation2IdOutside02;
        uint32_t worldLocation2IdOutside03;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            housingDecorInfoIdDefaultRoof = file.getUint(recordIndex, i++);
            housingDecorInfoIdDefaultEntryway = file.getUint(recordIndex, i++);
            housingDecorInfoIdDefaultDoor = file.getUint(recordIndex, i++);
            housingWallpaperInfoIdDefault = file.getUint(recordIndex, i++);
            worldLocation2IdInside00 = file.getUint(recordIndex, i++);
            worldLocation2IdInside01 = file.getUint(recordIndex, i++);
            worldLocation2IdInside02 = file.getUint(recordIndex, i++);
            worldLocation2IdInside03 = file.getUint(recordIndex, i++);
            worldLocation2IdOutside00 = file.getUint(recordIndex, i++);
            worldLocation2IdOutside01 = file.getUint(recordIndex, i++);
            worldLocation2IdOutside02 = file.getUint(recordIndex, i++);
            worldLocation2IdOutside03 = file.getUint(recordIndex, i++);
        }
    };
}
