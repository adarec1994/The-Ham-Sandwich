#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PublicEventVote
    {
        uint32_t ID;
        uint32_t localizedTextIdTitle;
        uint32_t localizedTextIdDescription;
        uint32_t localizedTextIdOption00;
        uint32_t localizedTextIdOption01;
        uint32_t localizedTextIdOption02;
        uint32_t localizedTextIdOption03;
        uint32_t localizedTextIdOption04;
        uint32_t localizedTextIdLabel00;
        uint32_t localizedTextIdLabel01;
        uint32_t localizedTextIdLabel02;
        uint32_t localizedTextIdLabel03;
        uint32_t localizedTextIdLabel04;
        uint32_t durationMS;
        std::wstring assetPathSprite;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            localizedTextIdTitle = f.getUint(row, i++);
            localizedTextIdDescription = f.getUint(row, i++);
            localizedTextIdOption00 = f.getUint(row, i++);
            localizedTextIdOption01 = f.getUint(row, i++);
            localizedTextIdOption02 = f.getUint(row, i++);
            localizedTextIdOption03 = f.getUint(row, i++);
            localizedTextIdOption04 = f.getUint(row, i++);
            localizedTextIdLabel00 = f.getUint(row, i++);
            localizedTextIdLabel01 = f.getUint(row, i++);
            localizedTextIdLabel02 = f.getUint(row, i++);
            localizedTextIdLabel03 = f.getUint(row, i++);
            localizedTextIdLabel04 = f.getUint(row, i++);
            durationMS = f.getUint(row, i++);
            assetPathSprite = f.getString(row, i++);
        }
    };
}
