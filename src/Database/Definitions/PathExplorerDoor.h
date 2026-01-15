#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct PathExplorerDoor
    {
        uint32_t ID;
        uint32_t worldZoneIdInsideMicro;
        uint32_t targetGroupIdActivate;
        uint32_t targetGroupIdKill;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            worldZoneIdInsideMicro = file.getUint(recordIndex, i++);
            targetGroupIdActivate = file.getUint(recordIndex, i++);
            targetGroupIdKill = file.getUint(recordIndex, i++);
        }
    };
}
