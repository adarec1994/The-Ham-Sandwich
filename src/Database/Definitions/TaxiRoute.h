#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TaxiRoute
    {
        static constexpr const char* GetFileName() { return "TaxiRoute"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t taxiNodeIdSource;
        uint32_t taxiNodeIdDestination;
        uint32_t price;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            taxiNodeIdSource = file.getUint(recordIndex, col++);
            taxiNodeIdDestination = file.getUint(recordIndex, col++);
            price = file.getUint(recordIndex, col++);
        }
    };
}