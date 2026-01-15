#pragma once
#include <cstdint>
#include <string>
#include "../Tbl.h"

namespace Database::Definitions
{
    struct Spell4VisualGroup
    {
        static constexpr const char* GetFileName() { return "Spell4VisualGroup"; }
        uint32_t GetID() const { return ID; }

        uint32_t ID;
        uint32_t spell4VisualIdVisual00;
        uint32_t spell4VisualIdVisual01;
        uint32_t spell4VisualIdVisual02;
        uint32_t spell4VisualIdVisual03;
        uint32_t spell4VisualIdVisual04;
        uint32_t spell4VisualIdVisual05;
        uint32_t spell4VisualIdVisual06;
        uint32_t spell4VisualIdVisual07;
        uint32_t spell4VisualIdVisual08;
        uint32_t spell4VisualIdVisual09;
        uint32_t spell4VisualIdVisual10;
        uint32_t spell4VisualIdVisual11;
        uint32_t spell4VisualIdVisual12;
        uint32_t spell4VisualIdVisual13;
        uint32_t spell4VisualIdVisual14;
        uint32_t spell4VisualIdVisual15;
        uint32_t spell4VisualIdVisual16;
        uint32_t spell4VisualIdVisual17;
        uint32_t spell4VisualIdVisual18;
        uint32_t spell4VisualIdVisual19;
        uint32_t spell4VisualIdVisual20;
        uint32_t spell4VisualIdVisual21;
        uint32_t spell4VisualIdVisual22;
        uint32_t spell4VisualIdVisual23;
        uint32_t spell4VisualIdVisual24;
        uint32_t spell4VisualIdVisual25;
        uint32_t spell4VisualIdVisual26;
        uint32_t spell4VisualIdVisual27;
        uint32_t spell4VisualIdVisual28;
        uint32_t spell4VisualIdVisual29;
        uint32_t spell4VisualIdVisual30;
        uint32_t spell4VisualIdVisual31;
        uint32_t spell4VisualIdVisual32;
        uint32_t spell4VisualIdVisual33;
        uint32_t spell4VisualIdVisual34;
        uint32_t spell4VisualIdVisual35;
        uint32_t visualEffectIdPrimaryCaster;

        void Read(const Tbl::File& file, uint32_t recordIndex)
        {
            size_t col = 0;
            ID = file.getUint(recordIndex, col++);
            spell4VisualIdVisual00 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual01 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual02 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual03 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual04 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual05 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual06 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual07 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual08 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual09 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual10 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual11 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual12 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual13 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual14 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual15 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual16 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual17 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual18 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual19 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual20 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual21 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual22 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual23 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual24 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual25 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual26 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual27 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual28 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual29 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual30 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual31 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual32 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual33 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual34 = file.getUint(recordIndex, col++);
            spell4VisualIdVisual35 = file.getUint(recordIndex, col++);
            visualEffectIdPrimaryCaster = file.getUint(recordIndex, col++);
        }
    };
}