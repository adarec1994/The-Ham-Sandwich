#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct EpisodeQuest
    {
        uint32_t ID;
        uint32_t episodeId;
        uint32_t questId;
        uint32_t orderIdx;
        uint32_t flags;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            episodeId = file.getUint(recordIndex, i++);
            questId = file.getUint(recordIndex, i++);
            orderIdx = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
        }
    };
}
