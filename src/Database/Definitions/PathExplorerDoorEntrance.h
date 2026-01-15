#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct PathExplorerDoorEntrance
    {
        uint32_t ID;
        uint32_t pathExplorerDoorTypeEnumSurface;
        uint32_t pathExplorerDoorTypeEnumMicro;
        uint32_t creature2IdSurface;
        uint32_t creature2IdMicro;
        uint32_t pathExplorerDoorId;
        uint32_t worldLocation2IdSurfaceRevealed;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            pathExplorerDoorTypeEnumSurface = file.getUint(recordIndex, i++);
            pathExplorerDoorTypeEnumMicro = file.getUint(recordIndex, i++);
            creature2IdSurface = file.getUint(recordIndex, i++);
            creature2IdMicro = file.getUint(recordIndex, i++);
            pathExplorerDoorId = file.getUint(recordIndex, i++);
            worldLocation2IdSurfaceRevealed = file.getUint(recordIndex, i++);
        }
    };
}
