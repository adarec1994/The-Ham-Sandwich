#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct PathScientistFieldStudy
    {
        uint32_t ID;
        uint32_t creature2Id;
        uint32_t targetGroupId;
        uint32_t pathScientistFieldStudyFlags;
        uint32_t localizedTextIdChecklist00;
        uint32_t localizedTextIdChecklist01;
        uint32_t localizedTextIdChecklist02;
        uint32_t localizedTextIdChecklist03;
        uint32_t localizedTextIdChecklist04;
        uint32_t localizedTextIdChecklist05;
        uint32_t localizedTextIdChecklist06;
        uint32_t localizedTextIdChecklist07;
        uint32_t worldLocation2IdIndicator00;
        uint32_t worldLocation2IdIndicator01;
        uint32_t worldLocation2IdIndicator02;
        uint32_t worldLocation2IdIndicator03;
        uint32_t worldLocation2IdIndicator04;
        uint32_t worldLocation2IdIndicator05;
        uint32_t worldLocation2IdIndicator06;
        uint32_t worldLocation2IdIndicator07;
        uint32_t behaviorType00;
        uint32_t behaviorType01;
        uint32_t behaviorType02;
        uint32_t behaviorType03;
        uint32_t behaviorType04;
        uint32_t behaviorType05;
        uint32_t behaviorType06;
        uint32_t behaviorType07;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            creature2Id = file.getUint(recordIndex, i++);
            targetGroupId = file.getUint(recordIndex, i++);
            pathScientistFieldStudyFlags = file.getUint(recordIndex, i++);
            localizedTextIdChecklist00 = file.getUint(recordIndex, i++);
            localizedTextIdChecklist01 = file.getUint(recordIndex, i++);
            localizedTextIdChecklist02 = file.getUint(recordIndex, i++);
            localizedTextIdChecklist03 = file.getUint(recordIndex, i++);
            localizedTextIdChecklist04 = file.getUint(recordIndex, i++);
            localizedTextIdChecklist05 = file.getUint(recordIndex, i++);
            localizedTextIdChecklist06 = file.getUint(recordIndex, i++);
            localizedTextIdChecklist07 = file.getUint(recordIndex, i++);
            worldLocation2IdIndicator00 = file.getUint(recordIndex, i++);
            worldLocation2IdIndicator01 = file.getUint(recordIndex, i++);
            worldLocation2IdIndicator02 = file.getUint(recordIndex, i++);
            worldLocation2IdIndicator03 = file.getUint(recordIndex, i++);
            worldLocation2IdIndicator04 = file.getUint(recordIndex, i++);
            worldLocation2IdIndicator05 = file.getUint(recordIndex, i++);
            worldLocation2IdIndicator06 = file.getUint(recordIndex, i++);
            worldLocation2IdIndicator07 = file.getUint(recordIndex, i++);
            behaviorType00 = file.getUint(recordIndex, i++);
            behaviorType01 = file.getUint(recordIndex, i++);
            behaviorType02 = file.getUint(recordIndex, i++);
            behaviorType03 = file.getUint(recordIndex, i++);
            behaviorType04 = file.getUint(recordIndex, i++);
            behaviorType05 = file.getUint(recordIndex, i++);
            behaviorType06 = file.getUint(recordIndex, i++);
            behaviorType07 = file.getUint(recordIndex, i++);
        }
    };
}
