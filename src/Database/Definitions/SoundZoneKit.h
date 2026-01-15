#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct SoundZoneKit
    {
        uint32_t ID;
        uint32_t soundZoneKitIdParent;
        uint32_t worldZoneId;
        uint32_t inheritFlags;
        uint32_t propertyFlags;
        uint32_t soundMusicSetId;
        uint32_t soundEventIdIntro;
        float introReplayWait;
        uint32_t soundEventIdMusicMood;
        uint32_t soundEventIdAmbientDay;
        uint32_t soundEventIdAmbientNight;
        uint32_t soundEventIdAmbientUnderwater;
        uint32_t soundEventIdAmbientStop;
        uint32_t soundEventIdAmbientPreStopOverride;
        uint32_t soundEnvironmentId00;
        uint32_t soundEnvironmentId01;
        float environmentDry;
        float environmentWet00;
        float environmentWet01;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            soundZoneKitIdParent = f.getUint(row, i++);
            worldZoneId = f.getUint(row, i++);
            inheritFlags = f.getUint(row, i++);
            propertyFlags = f.getUint(row, i++);
            soundMusicSetId = f.getUint(row, i++);
            soundEventIdIntro = f.getUint(row, i++);
            introReplayWait = f.getFloat(row, i++);
            soundEventIdMusicMood = f.getUint(row, i++);
            soundEventIdAmbientDay = f.getUint(row, i++);
            soundEventIdAmbientNight = f.getUint(row, i++);
            soundEventIdAmbientUnderwater = f.getUint(row, i++);
            soundEventIdAmbientStop = f.getUint(row, i++);
            soundEventIdAmbientPreStopOverride = f.getUint(row, i++);
            soundEnvironmentId00 = f.getUint(row, i++);
            soundEnvironmentId01 = f.getUint(row, i++);
            environmentDry = f.getFloat(row, i++);
            environmentWet00 = f.getFloat(row, i++);
            environmentWet01 = f.getFloat(row, i++);
        }
    };
}
