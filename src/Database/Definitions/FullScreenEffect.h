#pragma once

#include "../Tbl.h"
#include <cstdint>
#include <string>

namespace Tbl
{
    struct FullScreenEffect
    {
        uint32_t ID;
        std::wstring description;
        std::wstring texturePath;
        std::wstring modelPath;
        uint32_t priority;
        uint32_t fullScreenEffectTypeEnum;
        float alphaMinStart;
        float alphaMinEnd;
        float alphaStart;
        float alphaEnd;
        float hzStart;
        float hzEnd;
        float saturationStart;
        float saturationEnd;

        uint32_t GetID() const { return ID; }

        void Read(const File& file, uint32_t recordIndex)
        {
            size_t i = 0;
            ID = file.getUint(recordIndex, i++);
            description = file.getString(recordIndex, i++);
            texturePath = file.getString(recordIndex, i++);
            modelPath = file.getString(recordIndex, i++);
            priority = file.getUint(recordIndex, i++);
            fullScreenEffectTypeEnum = file.getUint(recordIndex, i++);
            alphaMinStart = file.getFloat(recordIndex, i++);
            alphaMinEnd = file.getFloat(recordIndex, i++);
            alphaStart = file.getFloat(recordIndex, i++);
            alphaEnd = file.getFloat(recordIndex, i++);
            hzStart = file.getFloat(recordIndex, i++);
            hzEnd = file.getFloat(recordIndex, i++);
            saturationStart = file.getFloat(recordIndex, i++);
            saturationEnd = file.getFloat(recordIndex, i++);
        }
    };
}
