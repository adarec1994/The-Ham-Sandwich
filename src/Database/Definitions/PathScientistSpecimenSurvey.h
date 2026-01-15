#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct PathScientistSpecimenSurvey
    {
        uint32_t ID;
        uint32_t localizedTextIdObjective00;
        uint32_t localizedTextIdObjective01;
        uint32_t localizedTextIdObjective02;
        uint32_t localizedTextIdObjective03;
        uint32_t localizedTextIdObjective04;
        uint32_t localizedTextIdObjective05;
        uint32_t localizedTextIdObjective06;
        uint32_t localizedTextIdObjective07;
        uint32_t localizedTextIdObjective08;
        uint32_t localizedTextIdObjective09;
        uint32_t worldLocation2Id00;
        uint32_t worldLocation2Id01;
        uint32_t worldLocation2Id02;
        uint32_t worldLocation2Id03;
        uint32_t worldLocation2Id04;
        uint32_t worldLocation2Id05;
        uint32_t worldLocation2Id06;
        uint32_t worldLocation2Id07;
        uint32_t worldLocation2Id08;
        uint32_t worldLocation2Id09;
        uint32_t specimenSurveyFlags00;
        uint32_t specimenSurveyFlags01;
        uint32_t specimenSurveyFlags02;
        uint32_t specimenSurveyFlags03;
        uint32_t specimenSurveyFlags04;
        uint32_t specimenSurveyFlags05;
        uint32_t specimenSurveyFlags06;
        uint32_t specimenSurveyFlags07;
        uint32_t specimenSurveyFlags08;
        uint32_t specimenSurveyFlags09;
        uint32_t questDirectionId00;
        uint32_t questDirectionId01;
        uint32_t questDirectionId02;
        uint32_t questDirectionId03;
        uint32_t questDirectionId04;
        uint32_t questDirectionId05;
        uint32_t questDirectionId06;
        uint32_t questDirectionId07;
        uint32_t questDirectionId08;
        uint32_t questDirectionId09;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdObjective00 = file.getUint(recordIndex, i++);
            localizedTextIdObjective01 = file.getUint(recordIndex, i++);
            localizedTextIdObjective02 = file.getUint(recordIndex, i++);
            localizedTextIdObjective03 = file.getUint(recordIndex, i++);
            localizedTextIdObjective04 = file.getUint(recordIndex, i++);
            localizedTextIdObjective05 = file.getUint(recordIndex, i++);
            localizedTextIdObjective06 = file.getUint(recordIndex, i++);
            localizedTextIdObjective07 = file.getUint(recordIndex, i++);
            localizedTextIdObjective08 = file.getUint(recordIndex, i++);
            localizedTextIdObjective09 = file.getUint(recordIndex, i++);
            worldLocation2Id00 = file.getUint(recordIndex, i++);
            worldLocation2Id01 = file.getUint(recordIndex, i++);
            worldLocation2Id02 = file.getUint(recordIndex, i++);
            worldLocation2Id03 = file.getUint(recordIndex, i++);
            worldLocation2Id04 = file.getUint(recordIndex, i++);
            worldLocation2Id05 = file.getUint(recordIndex, i++);
            worldLocation2Id06 = file.getUint(recordIndex, i++);
            worldLocation2Id07 = file.getUint(recordIndex, i++);
            worldLocation2Id08 = file.getUint(recordIndex, i++);
            worldLocation2Id09 = file.getUint(recordIndex, i++);
            specimenSurveyFlags00 = file.getUint(recordIndex, i++);
            specimenSurveyFlags01 = file.getUint(recordIndex, i++);
            specimenSurveyFlags02 = file.getUint(recordIndex, i++);
            specimenSurveyFlags03 = file.getUint(recordIndex, i++);
            specimenSurveyFlags04 = file.getUint(recordIndex, i++);
            specimenSurveyFlags05 = file.getUint(recordIndex, i++);
            specimenSurveyFlags06 = file.getUint(recordIndex, i++);
            specimenSurveyFlags07 = file.getUint(recordIndex, i++);
            specimenSurveyFlags08 = file.getUint(recordIndex, i++);
            specimenSurveyFlags09 = file.getUint(recordIndex, i++);
            questDirectionId00 = file.getUint(recordIndex, i++);
            questDirectionId01 = file.getUint(recordIndex, i++);
            questDirectionId02 = file.getUint(recordIndex, i++);
            questDirectionId03 = file.getUint(recordIndex, i++);
            questDirectionId04 = file.getUint(recordIndex, i++);
            questDirectionId05 = file.getUint(recordIndex, i++);
            questDirectionId06 = file.getUint(recordIndex, i++);
            questDirectionId07 = file.getUint(recordIndex, i++);
            questDirectionId08 = file.getUint(recordIndex, i++);
            questDirectionId09 = file.getUint(recordIndex, i++);
        }
    };
}
