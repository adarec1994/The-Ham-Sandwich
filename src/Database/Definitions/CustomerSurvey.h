#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct CustomerSurvey
    {
        uint32_t ID;
        uint32_t customerSurveyTypeEnum;
        uint32_t localizedTextIdOverrideTitle;
        uint32_t localizedTextIdQuestion00;
        uint32_t localizedTextIdQuestion01;
        uint32_t localizedTextIdQuestion02;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            customerSurveyTypeEnum = file.getUint(recordIndex, i++);
            localizedTextIdOverrideTitle = file.getUint(recordIndex, i++);
            localizedTextIdQuestion00 = file.getUint(recordIndex, i++);
            localizedTextIdQuestion01 = file.getUint(recordIndex, i++);
            localizedTextIdQuestion02 = file.getUint(recordIndex, i++);
        }
    };
}
