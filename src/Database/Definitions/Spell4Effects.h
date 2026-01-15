#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct Spell4Effects
    {
        uint32_t ID;
        uint32_t spellId;
        uint32_t targetFlags;
        uint32_t effectType;
        uint32_t damageType;
        uint32_t delayTime;
        uint32_t tickTime;
        uint32_t durationTime;
        uint32_t flags;
        uint32_t dataBits00;
        uint32_t dataBits01;
        uint32_t dataBits02;
        uint32_t dataBits03;
        uint32_t dataBits04;
        uint32_t dataBits05;
        uint32_t dataBits06;
        uint32_t dataBits07;
        uint32_t dataBits08;
        uint32_t dataBits09;
        uint32_t innateCostPerTickType0;
        uint32_t innateCostPerTickType1;
        uint32_t innateCostPerTick0;
        uint32_t innateCostPerTick1;
        uint32_t emmComparison;
        uint32_t emmValue;
        float threatMultiplier;
        uint32_t spell4EffectGroupListId;
        uint32_t prerequisiteIdCasterApply;
        uint32_t prerequisiteIdTargetApply;
        uint32_t prerequisiteIdCasterPersistence;
        uint32_t prerequisiteIdTargetPersistence;
        uint32_t prerequisiteIdTargetSuspend;
        uint32_t parameterType00;
        uint32_t parameterType01;
        uint32_t parameterType02;
        uint32_t parameterType03;
        float parameterValue00;
        float parameterValue01;
        float parameterValue02;
        float parameterValue03;
        uint32_t phaseFlags;
        uint32_t orderIndex;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            spellId = f.getUint(row, i++);
            targetFlags = f.getUint(row, i++);
            effectType = f.getUint(row, i++);
            damageType = f.getUint(row, i++);
            delayTime = f.getUint(row, i++);
            tickTime = f.getUint(row, i++);
            durationTime = f.getUint(row, i++);
            flags = f.getUint(row, i++);
            dataBits00 = f.getUint(row, i++);
            dataBits01 = f.getUint(row, i++);
            dataBits02 = f.getUint(row, i++);
            dataBits03 = f.getUint(row, i++);
            dataBits04 = f.getUint(row, i++);
            dataBits05 = f.getUint(row, i++);
            dataBits06 = f.getUint(row, i++);
            dataBits07 = f.getUint(row, i++);
            dataBits08 = f.getUint(row, i++);
            dataBits09 = f.getUint(row, i++);
            innateCostPerTickType0 = f.getUint(row, i++);
            innateCostPerTickType1 = f.getUint(row, i++);
            innateCostPerTick0 = f.getUint(row, i++);
            innateCostPerTick1 = f.getUint(row, i++);
            emmComparison = f.getUint(row, i++);
            emmValue = f.getUint(row, i++);
            threatMultiplier = f.getFloat(row, i++);
            spell4EffectGroupListId = f.getUint(row, i++);
            prerequisiteIdCasterApply = f.getUint(row, i++);
            prerequisiteIdTargetApply = f.getUint(row, i++);
            prerequisiteIdCasterPersistence = f.getUint(row, i++);
            prerequisiteIdTargetPersistence = f.getUint(row, i++);
            prerequisiteIdTargetSuspend = f.getUint(row, i++);
            parameterType00 = f.getUint(row, i++);
            parameterType01 = f.getUint(row, i++);
            parameterType02 = f.getUint(row, i++);
            parameterType03 = f.getUint(row, i++);
            parameterValue00 = f.getFloat(row, i++);
            parameterValue01 = f.getFloat(row, i++);
            parameterValue02 = f.getFloat(row, i++);
            parameterValue03 = f.getFloat(row, i++);
            phaseFlags = f.getUint(row, i++);
            orderIndex = f.getUint(row, i++);
        }
    };
}
