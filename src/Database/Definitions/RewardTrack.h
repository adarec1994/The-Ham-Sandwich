#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct RewardTrack
    {
        uint32_t ID;
        uint32_t rewardTrackTypeEnum;
        uint32_t prerequisiteId;
        uint32_t localizedTextId;
        uint32_t localizedTextIdSummary;
        std::wstring assetPathImage;
        std::wstring assetPathButtonImage;
        uint32_t rewardPointCost00;
        uint32_t rewardPointCost01;
        uint32_t rewardPointCost02;
        uint32_t rewardPointCost03;
        uint32_t rewardPointCost04;
        uint32_t rewardPointCost05;
        uint32_t rewardPointCost06;
        uint32_t rewardPointCost07;
        uint32_t rewardPointCost08;
        uint32_t rewardPointCost09;
        uint32_t rewardTrackIdParent;
        uint32_t flags;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            rewardTrackTypeEnum = f.getUint(row, i++);
            prerequisiteId = f.getUint(row, i++);
            localizedTextId = f.getUint(row, i++);
            localizedTextIdSummary = f.getUint(row, i++);
            assetPathImage = f.getString(row, i++);
            assetPathButtonImage = f.getString(row, i++);
            rewardPointCost00 = f.getUint(row, i++);
            rewardPointCost01 = f.getUint(row, i++);
            rewardPointCost02 = f.getUint(row, i++);
            rewardPointCost03 = f.getUint(row, i++);
            rewardPointCost04 = f.getUint(row, i++);
            rewardPointCost05 = f.getUint(row, i++);
            rewardPointCost06 = f.getUint(row, i++);
            rewardPointCost07 = f.getUint(row, i++);
            rewardPointCost08 = f.getUint(row, i++);
            rewardPointCost09 = f.getUint(row, i++);
            rewardTrackIdParent = f.getUint(row, i++);
            flags = f.getUint(row, i++);
        }
    };
}
