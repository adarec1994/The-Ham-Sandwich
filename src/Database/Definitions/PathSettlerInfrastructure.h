#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PathSettlerInfrastructure
    {
        uint32_t ID;
        uint32_t pathSettlerHubId00;
        uint32_t pathSettlerHubId01;
        uint32_t localizedTextIdObjective;
        uint32_t missionCount;
        uint32_t worldZoneId;
        uint32_t count;
        uint32_t maxTime;
        uint32_t creature2IdDepot;
        uint32_t creature2IdResource00;
        uint32_t creature2IdResource01;
        uint32_t creature2IdResource02;
        uint32_t spell4IdResource00;
        uint32_t spell4IdResource01;
        uint32_t spell4IdResource02;
        uint32_t maxStackCountResource00;
        uint32_t maxStackCountResource01;
        uint32_t maxStackCountResource02;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            pathSettlerHubId00 = f.getUint(row, i++);
            pathSettlerHubId01 = f.getUint(row, i++);
            localizedTextIdObjective = f.getUint(row, i++);
            missionCount = f.getUint(row, i++);
            worldZoneId = f.getUint(row, i++);
            count = f.getUint(row, i++);
            maxTime = f.getUint(row, i++);
            creature2IdDepot = f.getUint(row, i++);
            creature2IdResource00 = f.getUint(row, i++);
            creature2IdResource01 = f.getUint(row, i++);
            creature2IdResource02 = f.getUint(row, i++);
            spell4IdResource00 = f.getUint(row, i++);
            spell4IdResource01 = f.getUint(row, i++);
            spell4IdResource02 = f.getUint(row, i++);
            maxStackCountResource00 = f.getUint(row, i++);
            maxStackCountResource01 = f.getUint(row, i++);
            maxStackCountResource02 = f.getUint(row, i++);
        }
    };
}
