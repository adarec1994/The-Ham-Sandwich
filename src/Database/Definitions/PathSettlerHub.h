#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct PathSettlerHub
    {
        uint32_t ID;
        uint32_t maxAvenueEconomy;
        uint32_t maxAvenueSecurity;
        uint32_t maxAvenueQualityOfLife;
        uint32_t localizedTextIdEconomy;
        uint32_t localizedTextIdSecurity;
        uint32_t localizedTextIdQualityOfLife;
        uint32_t worldZoneId;
        uint32_t missionCount;
        uint32_t flags;
        uint32_t item2IdResource00;
        uint32_t item2IdResource01;
        uint32_t item2IdResource02;
        uint32_t publicEventObjectiveIdResource00;
        uint32_t publicEventObjectiveIdResource01;
        uint32_t publicEventObjectiveIdResource02;
        uint32_t worldLocation2IdMapResource00Loc00;
        uint32_t worldLocation2IdMapResource00Loc01;
        uint32_t worldLocation2IdMapResource00Loc02;
        uint32_t worldLocation2IdMapResource00Loc03;
        uint32_t worldLocation2IdMapResource01Loc00;
        uint32_t worldLocation2IdMapResource01Loc01;
        uint32_t worldLocation2IdMapResource01Loc02;
        uint32_t worldLocation2IdMapResource01Loc03;
        uint32_t worldLocation2IdMapResource02Loc00;
        uint32_t worldLocation2IdMapResource02Loc01;
        uint32_t worldLocation2IdMapResource02Loc02;
        uint32_t worldLocation2IdMapResource02Loc03;
        uint32_t localizedTextIdRewardNotify;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            maxAvenueEconomy = file.getUint(recordIndex, i++);
            maxAvenueSecurity = file.getUint(recordIndex, i++);
            maxAvenueQualityOfLife = file.getUint(recordIndex, i++);
            localizedTextIdEconomy = file.getUint(recordIndex, i++);
            localizedTextIdSecurity = file.getUint(recordIndex, i++);
            localizedTextIdQualityOfLife = file.getUint(recordIndex, i++);
            worldZoneId = file.getUint(recordIndex, i++);
            missionCount = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            item2IdResource00 = file.getUint(recordIndex, i++);
            item2IdResource01 = file.getUint(recordIndex, i++);
            item2IdResource02 = file.getUint(recordIndex, i++);
            publicEventObjectiveIdResource00 = file.getUint(recordIndex, i++);
            publicEventObjectiveIdResource01 = file.getUint(recordIndex, i++);
            publicEventObjectiveIdResource02 = file.getUint(recordIndex, i++);
            worldLocation2IdMapResource00Loc00 = file.getUint(recordIndex, i++);
            worldLocation2IdMapResource00Loc01 = file.getUint(recordIndex, i++);
            worldLocation2IdMapResource00Loc02 = file.getUint(recordIndex, i++);
            worldLocation2IdMapResource00Loc03 = file.getUint(recordIndex, i++);
            worldLocation2IdMapResource01Loc00 = file.getUint(recordIndex, i++);
            worldLocation2IdMapResource01Loc01 = file.getUint(recordIndex, i++);
            worldLocation2IdMapResource01Loc02 = file.getUint(recordIndex, i++);
            worldLocation2IdMapResource01Loc03 = file.getUint(recordIndex, i++);
            worldLocation2IdMapResource02Loc00 = file.getUint(recordIndex, i++);
            worldLocation2IdMapResource02Loc01 = file.getUint(recordIndex, i++);
            worldLocation2IdMapResource02Loc02 = file.getUint(recordIndex, i++);
            worldLocation2IdMapResource02Loc03 = file.getUint(recordIndex, i++);
            localizedTextIdRewardNotify = file.getUint(recordIndex, i++);
        }
    };
}
