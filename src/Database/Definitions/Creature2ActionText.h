#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Creature2ActionText
    {
        uint32_t ID;
        uint32_t localizedTextIdOnEnterCombat[4];
        float chanceToSayOnEnterCombat;
        uint32_t localizedTextIdOnDeath[4];
        float chanceToSayOnDeath;
        uint32_t localizedTextIdOnKill[4];
        float chanceToSayOnKill;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            for (int j = 0; j < 4; ++j)
                localizedTextIdOnEnterCombat[j] = file.getUint(recordIndex, i++);
            chanceToSayOnEnterCombat = file.getFloat(recordIndex, i++);
            for (int j = 0; j < 4; ++j)
                localizedTextIdOnDeath[j] = file.getUint(recordIndex, i++);
            chanceToSayOnDeath = file.getFloat(recordIndex, i++);
            for (int j = 0; j < 4; ++j)
                localizedTextIdOnKill[j] = file.getUint(recordIndex, i++);
            chanceToSayOnKill = file.getFloat(recordIndex, i++);
        }
    };
}
