#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TutorialPage
    {
        static constexpr const char* GetFileName() { return "TutorialPage"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t tutorialId;
        uint32_t page;
        uint32_t tutorialLayoutId;
        uint32_t localizedTextIdTitle;
        uint32_t localizedTextIdBody00;
        uint32_t localizedTextIdBody01;
        uint32_t localizedTextIdBody02;
        std::wstring sprite00;
        std::wstring sprite01;
        std::wstring sprite02;
        uint32_t soundEventId;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            tutorialId = file.getUint(recordIndex, col++);
            page = file.getUint(recordIndex, col++);
            tutorialLayoutId = file.getUint(recordIndex, col++);
            localizedTextIdTitle = file.getUint(recordIndex, col++);
            localizedTextIdBody00 = file.getUint(recordIndex, col++);
            localizedTextIdBody01 = file.getUint(recordIndex, col++);
            localizedTextIdBody02 = file.getUint(recordIndex, col++);
            sprite00 = file.getString(recordIndex, col++);
            sprite01 = file.getString(recordIndex, col++);
            sprite02 = file.getString(recordIndex, col++);
            soundEventId = file.getUint(recordIndex, col++);
        }
    };
}