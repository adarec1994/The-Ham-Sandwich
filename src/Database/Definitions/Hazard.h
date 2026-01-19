#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct Hazard
    {
        uint32_t ID;
        uint32_t localizedTextIdName;
        uint32_t localizedTextIdTooltip;
        float meterChangeRate;
        uint32_t meterMaxValue;
        uint32_t flags;
        uint32_t hazardTypeEnum;
        uint32_t spell4IdDamage;
        float minDistanceToUnit;
        float meterThreshold00;
        float meterThreshold01;
        float meterThreshold02;
        uint32_t spell4IdThresholdProc00;
        uint32_t spell4IdThresholdProc01;
        uint32_t spell4IdThresholdProc02;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdName = file.getUint(recordIndex, i++);
            localizedTextIdTooltip = file.getUint(recordIndex, i++);
            meterChangeRate = file.getFloat(recordIndex, i++);
            meterMaxValue = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            hazardTypeEnum = file.getUint(recordIndex, i++);
            spell4IdDamage = file.getUint(recordIndex, i++);
            minDistanceToUnit = file.getFloat(recordIndex, i++);
            meterThreshold00 = file.getFloat(recordIndex, i++);
            meterThreshold01 = file.getFloat(recordIndex, i++);
            meterThreshold02 = file.getFloat(recordIndex, i++);
            spell4IdThresholdProc00 = file.getUint(recordIndex, i++);
            spell4IdThresholdProc01 = file.getUint(recordIndex, i++);
            spell4IdThresholdProc02 = file.getUint(recordIndex, i++);
        }
    };
}
