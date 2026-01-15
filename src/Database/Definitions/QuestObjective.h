#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct QuestObjective
    {
        uint32_t ID;
        uint32_t type;
        uint32_t flags;
        uint32_t data;
        uint32_t count;
        uint32_t localizedTextIdFull;
        uint32_t worldLocationsIdIndicator00;
        uint32_t worldLocationsIdIndicator01;
        uint32_t worldLocationsIdIndicator02;
        uint32_t worldLocationsIdIndicator03;
        uint32_t maxTimeAllowedMS;
        uint32_t localizedTextIdShort;
        uint32_t targetGroupIdRewardPane;
        uint32_t questDirectionId;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            type = f.getUint(row, i++);
            flags = f.getUint(row, i++);
            data = f.getUint(row, i++);
            count = f.getUint(row, i++);
            localizedTextIdFull = f.getUint(row, i++);
            worldLocationsIdIndicator00 = f.getUint(row, i++);
            worldLocationsIdIndicator01 = f.getUint(row, i++);
            worldLocationsIdIndicator02 = f.getUint(row, i++);
            worldLocationsIdIndicator03 = f.getUint(row, i++);
            maxTimeAllowedMS = f.getUint(row, i++);
            localizedTextIdShort = f.getUint(row, i++);
            targetGroupIdRewardPane = f.getUint(row, i++);
            questDirectionId = f.getUint(row, i++);
        }
    };
}
