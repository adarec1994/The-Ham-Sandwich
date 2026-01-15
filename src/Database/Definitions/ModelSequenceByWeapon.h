#pragma once

#include "../Tbl.h"
#include <cstdint>

namespace Tbl
{
    struct ModelSequenceByWeapon
    {
        uint32_t ID;
        uint32_t modelSequenceId;
        uint32_t modelSequenceId1H;
        uint32_t modelSequenceId2H;
        uint32_t modelSequenceId2HL;
        uint32_t modelSequenceId2HGun;
        uint32_t modelSequenceIdPistols;
        uint32_t modelSequenceIdClaws;
        uint32_t modelSequenceIdShockPaddles;
        uint32_t modelSequenceIdEsper;
        uint32_t modelSequenceIdPsyblade;
        uint32_t modelSequenceIdHeavygun;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            modelSequenceId = file.getUint(recordIndex, i++);
            modelSequenceId1H = file.getUint(recordIndex, i++);
            modelSequenceId2H = file.getUint(recordIndex, i++);
            modelSequenceId2HL = file.getUint(recordIndex, i++);
            modelSequenceId2HGun = file.getUint(recordIndex, i++);
            modelSequenceIdPistols = file.getUint(recordIndex, i++);
            modelSequenceIdClaws = file.getUint(recordIndex, i++);
            modelSequenceIdShockPaddles = file.getUint(recordIndex, i++);
            modelSequenceIdEsper = file.getUint(recordIndex, i++);
            modelSequenceIdPsyblade = file.getUint(recordIndex, i++);
            modelSequenceIdHeavygun = file.getUint(recordIndex, i++);
        }
    };
}
