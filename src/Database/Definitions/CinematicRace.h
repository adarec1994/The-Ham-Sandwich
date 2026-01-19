#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct CinematicRace
    {
        uint32_t ID;
        uint32_t cinematicId;
        uint32_t raceId;
        std::wstring maleAssetPath;
        std::wstring femaleAssetPath;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            cinematicId = file.getUint(recordIndex, i++);
            raceId = file.getUint(recordIndex, i++);
            maleAssetPath = file.getString(recordIndex, i++);
            femaleAssetPath = file.getString(recordIndex, i++);
        }
    };
}
