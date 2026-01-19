#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct HousingBuild
    {
        uint32_t ID;
        std::wstring description;
        std::wstring assetPath;
        uint32_t constructionEffectsId;
        float buildPreDelayTimeMS;
        float buildPostDelayTimeMS;
        float buildTime00;
        float buildTime01;
        float buildTime02;
        float buildTime03;
        float buildTime04;
        float buildTime05;
        float buildTime06;
        float buildTime07;
        uint32_t modelSequenceId00;
        uint32_t modelSequenceId01;
        uint32_t modelSequenceId02;
        uint32_t modelSequenceId03;
        uint32_t modelSequenceId04;
        uint32_t modelSequenceId05;
        uint32_t modelSequenceId06;
        uint32_t modelSequenceId07;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            description = file.getString(recordIndex, i++);
            assetPath = file.getString(recordIndex, i++);
            constructionEffectsId = file.getUint(recordIndex, i++);
            buildPreDelayTimeMS = file.getFloat(recordIndex, i++);
            buildPostDelayTimeMS = file.getFloat(recordIndex, i++);
            buildTime00 = file.getFloat(recordIndex, i++);
            buildTime01 = file.getFloat(recordIndex, i++);
            buildTime02 = file.getFloat(recordIndex, i++);
            buildTime03 = file.getFloat(recordIndex, i++);
            buildTime04 = file.getFloat(recordIndex, i++);
            buildTime05 = file.getFloat(recordIndex, i++);
            buildTime06 = file.getFloat(recordIndex, i++);
            buildTime07 = file.getFloat(recordIndex, i++);
            modelSequenceId00 = file.getUint(recordIndex, i++);
            modelSequenceId01 = file.getUint(recordIndex, i++);
            modelSequenceId02 = file.getUint(recordIndex, i++);
            modelSequenceId03 = file.getUint(recordIndex, i++);
            modelSequenceId04 = file.getUint(recordIndex, i++);
            modelSequenceId05 = file.getUint(recordIndex, i++);
            modelSequenceId06 = file.getUint(recordIndex, i++);
            modelSequenceId07 = file.getUint(recordIndex, i++);
        }
    };
}
