#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct ZoneCompletion
    {
        static constexpr const char* GetFileName() { return "ZoneCompletion"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t mapZoneId;
        uint32_t zoneCompletionFactionEnum;
        uint32_t episodeQuestCount;
        uint32_t taskQuestCount;
        uint32_t challengeCount;
        uint32_t datacubeCount;
        uint32_t taleCount;
        uint32_t journalCount;
        uint32_t characterTitleIdReward;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            mapZoneId = file.getUint(recordIndex, col++);
            zoneCompletionFactionEnum = file.getUint(recordIndex, col++);
            episodeQuestCount = file.getUint(recordIndex, col++);
            taskQuestCount = file.getUint(recordIndex, col++);
            challengeCount = file.getUint(recordIndex, col++);
            datacubeCount = file.getUint(recordIndex, col++);
            taleCount = file.getUint(recordIndex, col++);
            journalCount = file.getUint(recordIndex, col++);
            characterTitleIdReward = file.getUint(recordIndex, col++);
        }
    };
}