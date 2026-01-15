#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PlayerNotificationType
    {
        uint32_t ID;
        uint32_t priority;
        uint32_t lifetimeMs;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            priority = f.getUint(row, i++);
            lifetimeMs = f.getUint(row, i++);
        }
    };
}
