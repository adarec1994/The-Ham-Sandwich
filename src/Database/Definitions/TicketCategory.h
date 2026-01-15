#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TicketCategory
    {
        static constexpr const char* GetFileName() { return "TicketCategory"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t localizedTextId;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            localizedTextId = file.getUint(recordIndex, col++);
        }
    };
}