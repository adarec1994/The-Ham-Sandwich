#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct TutorialLayout
    {
        static constexpr const char* GetFileName() { return "TutorialLayout"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        std::wstring form;
        uint32_t flags;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            form = file.getString(recordIndex, col++);
            flags = file.getUint(recordIndex, col++);
        }
    };
}