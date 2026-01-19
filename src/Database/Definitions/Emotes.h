#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct Emotes
    {
        uint32_t ID;
        uint32_t localizedTextIdNoArgToAll;
        uint32_t localizedTextIdNoArgToSelf;
        uint32_t NoArgAnim;
        uint32_t localizedTextIdArgToAll;
        uint32_t localizedTextIdArgToArg;
        uint32_t localizedTextIdArgToSelf;
        uint32_t ArgAnim;
        uint32_t localizedTextIdSelfToAll;
        uint32_t localizedTextIdSelfToSelf;
        uint32_t SelfAnim;
        uint32_t SheathWeapons;
        uint32_t TurnToFace;
        uint32_t TextReplaceable;
        uint32_t ChangesStandState;
        uint32_t StandState;
        uint32_t localizedTextIdCommand;
        uint32_t localizedTextIdNotFoundToAll;
        uint32_t localizedTextIdNotFoundToSelf;
        uint32_t NotFoundAnim;
        uint32_t TextReplaceAnim;
        uint32_t modelSequenceIdStandState;
        uint32_t visualEffectId;
        uint32_t flags;
        std::wstring universalCommand00;
        std::wstring universalCommand01;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            localizedTextIdNoArgToAll = file.getUint(recordIndex, i++);
            localizedTextIdNoArgToSelf = file.getUint(recordIndex, i++);
            NoArgAnim = file.getUint(recordIndex, i++);
            localizedTextIdArgToAll = file.getUint(recordIndex, i++);
            localizedTextIdArgToArg = file.getUint(recordIndex, i++);
            localizedTextIdArgToSelf = file.getUint(recordIndex, i++);
            ArgAnim = file.getUint(recordIndex, i++);
            localizedTextIdSelfToAll = file.getUint(recordIndex, i++);
            localizedTextIdSelfToSelf = file.getUint(recordIndex, i++);
            SelfAnim = file.getUint(recordIndex, i++);
            SheathWeapons = file.getUint(recordIndex, i++);
            TurnToFace = file.getUint(recordIndex, i++);
            TextReplaceable = file.getUint(recordIndex, i++);
            ChangesStandState = file.getUint(recordIndex, i++);
            StandState = file.getUint(recordIndex, i++);
            localizedTextIdCommand = file.getUint(recordIndex, i++);
            localizedTextIdNotFoundToAll = file.getUint(recordIndex, i++);
            localizedTextIdNotFoundToSelf = file.getUint(recordIndex, i++);
            NotFoundAnim = file.getUint(recordIndex, i++);
            TextReplaceAnim = file.getUint(recordIndex, i++);
            modelSequenceIdStandState = file.getUint(recordIndex, i++);
            visualEffectId = file.getUint(recordIndex, i++);
            flags = file.getUint(recordIndex, i++);
            universalCommand00 = file.getString(recordIndex, i++);
            universalCommand01 = file.getString(recordIndex, i++);
        }
    };
}
