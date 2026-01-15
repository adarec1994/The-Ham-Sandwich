#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PublicEventObjectiveGatherResource
    {
        uint32_t ID;
        uint32_t publicEventObjectiveGatherResourceEnumFlag;
        uint32_t creature2IdContainer;
        uint32_t creature2IdResource;
        uint32_t spell4IdResource;
        uint32_t creature2IdStolenResource;
        uint32_t spell4IdStolenResource;
        uint32_t publicEventObjectiveGatherResourceIdOpposing;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            publicEventObjectiveGatherResourceEnumFlag = f.getUint(row, i++);
            creature2IdContainer = f.getUint(row, i++);
            creature2IdResource = f.getUint(row, i++);
            spell4IdResource = f.getUint(row, i++);
            creature2IdStolenResource = f.getUint(row, i++);
            spell4IdStolenResource = f.getUint(row, i++);
            publicEventObjectiveGatherResourceIdOpposing = f.getUint(row, i++);
        }
    };
}
