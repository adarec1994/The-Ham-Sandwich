#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct PathSettlerSheriff
    {
        uint32_t ID;
        uint32_t quest2IdSheriff00;
        uint32_t quest2IdSheriff01;
        uint32_t quest2IdSheriff02;
        uint32_t quest2IdSheriff03;
        uint32_t quest2IdSheriff04;
        uint32_t quest2IdSheriff05;
        uint32_t quest2IdSheriff06;
        uint32_t quest2IdSheriff07;
        uint32_t localizedTextIdDescription00;
        uint32_t localizedTextIdDescription01;
        uint32_t localizedTextIdDescription02;
        uint32_t localizedTextIdDescription03;
        uint32_t localizedTextIdDescription04;
        uint32_t localizedTextIdDescription05;
        uint32_t localizedTextIdDescription06;
        uint32_t localizedTextIdDescription07;
        uint32_t characterTitleIdReward;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            quest2IdSheriff00 = f.getUint(row, i++);
            quest2IdSheriff01 = f.getUint(row, i++);
            quest2IdSheriff02 = f.getUint(row, i++);
            quest2IdSheriff03 = f.getUint(row, i++);
            quest2IdSheriff04 = f.getUint(row, i++);
            quest2IdSheriff05 = f.getUint(row, i++);
            quest2IdSheriff06 = f.getUint(row, i++);
            quest2IdSheriff07 = f.getUint(row, i++);
            localizedTextIdDescription00 = f.getUint(row, i++);
            localizedTextIdDescription01 = f.getUint(row, i++);
            localizedTextIdDescription02 = f.getUint(row, i++);
            localizedTextIdDescription03 = f.getUint(row, i++);
            localizedTextIdDescription04 = f.getUint(row, i++);
            localizedTextIdDescription05 = f.getUint(row, i++);
            localizedTextIdDescription06 = f.getUint(row, i++);
            localizedTextIdDescription07 = f.getUint(row, i++);
            characterTitleIdReward = f.getUint(row, i++);
        }
    };
}
