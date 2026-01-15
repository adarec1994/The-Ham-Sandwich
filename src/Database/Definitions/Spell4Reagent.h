#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct Spell4Reagent
    {
        static constexpr const char* GetFileName() { return "Spell4Reagent"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t reagentType;
        uint32_t reagentTypeObjectId;
        uint32_t reagentCount;
        uint32_t consumeReagent;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            reagentType = file.getUint(recordIndex, col++);
            reagentTypeObjectId = file.getUint(recordIndex, col++);
            reagentCount = file.getUint(recordIndex, col++);
            consumeReagent = file.getUint(recordIndex, col++);
        }
    };
}