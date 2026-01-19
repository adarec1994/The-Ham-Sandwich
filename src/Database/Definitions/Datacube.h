#pragma once
#include "../Tbl.h"
#include <string>

namespace Tbl
{
    struct Datacube
    {
        uint32_t ID;
        uint32_t datacubeTypeEnum;
        uint32_t localizedTextIdTitle;
        uint32_t localizedTextIdText00;
        uint32_t localizedTextIdText01;
        uint32_t localizedTextIdText02;
        uint32_t localizedTextIdText03;
        uint32_t localizedTextIdText04;
        uint32_t localizedTextIdText05;
        uint32_t soundEventId;
        uint32_t worldZoneId;
        uint32_t unlockCount;
        std::wstring assetPathImage;
        uint32_t datacubeFactionEnum;
        uint32_t worldLocation2Id;
        uint32_t questDirectionId;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            datacubeTypeEnum = file.getUint(recordIndex, i++);
            localizedTextIdTitle = file.getUint(recordIndex, i++);
            localizedTextIdText00 = file.getUint(recordIndex, i++);
            localizedTextIdText01 = file.getUint(recordIndex, i++);
            localizedTextIdText02 = file.getUint(recordIndex, i++);
            localizedTextIdText03 = file.getUint(recordIndex, i++);
            localizedTextIdText04 = file.getUint(recordIndex, i++);
            localizedTextIdText05 = file.getUint(recordIndex, i++);
            soundEventId = file.getUint(recordIndex, i++);
            worldZoneId = file.getUint(recordIndex, i++);
            unlockCount = file.getUint(recordIndex, i++);
            assetPathImage = file.getString(recordIndex, i++);
            datacubeFactionEnum = file.getUint(recordIndex, i++);
            worldLocation2Id = file.getUint(recordIndex, i++);
            questDirectionId = file.getUint(recordIndex, i++);
        }
    };
}
