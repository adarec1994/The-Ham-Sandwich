#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct SkyTrackCloudSet
    {
        uint32_t ID;
        uint32_t count;
        float minSize00;
        float minSize01;
        float minSize02;
        float minSize03;
        float minSize04;
        float minSize05;
        float minSize06;
        float minSize07;
        float minSize08;
        float minSize09;
        float minSize10;
        float minSize11;
        float maxSize00;
        float maxSize01;
        float maxSize02;
        float maxSize03;
        float maxSize04;
        float maxSize05;
        float maxSize06;
        float maxSize07;
        float maxSize08;
        float maxSize09;
        float maxSize10;
        float maxSize11;
        std::wstring model00;
        std::wstring model01;
        std::wstring model02;
        std::wstring model03;
        std::wstring model04;
        std::wstring model05;
        std::wstring model06;
        std::wstring model07;
        std::wstring model08;
        std::wstring model09;
        std::wstring model10;
        std::wstring model11;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            count = f.getUint(row, i++);
            minSize00 = f.getFloat(row, i++);
            minSize01 = f.getFloat(row, i++);
            minSize02 = f.getFloat(row, i++);
            minSize03 = f.getFloat(row, i++);
            minSize04 = f.getFloat(row, i++);
            minSize05 = f.getFloat(row, i++);
            minSize06 = f.getFloat(row, i++);
            minSize07 = f.getFloat(row, i++);
            minSize08 = f.getFloat(row, i++);
            minSize09 = f.getFloat(row, i++);
            minSize10 = f.getFloat(row, i++);
            minSize11 = f.getFloat(row, i++);
            maxSize00 = f.getFloat(row, i++);
            maxSize01 = f.getFloat(row, i++);
            maxSize02 = f.getFloat(row, i++);
            maxSize03 = f.getFloat(row, i++);
            maxSize04 = f.getFloat(row, i++);
            maxSize05 = f.getFloat(row, i++);
            maxSize06 = f.getFloat(row, i++);
            maxSize07 = f.getFloat(row, i++);
            maxSize08 = f.getFloat(row, i++);
            maxSize09 = f.getFloat(row, i++);
            maxSize10 = f.getFloat(row, i++);
            maxSize11 = f.getFloat(row, i++);
            model00 = f.getString(row, i++);
            model01 = f.getString(row, i++);
            model02 = f.getString(row, i++);
            model03 = f.getString(row, i++);
            model04 = f.getString(row, i++);
            model05 = f.getString(row, i++);
            model06 = f.getString(row, i++);
            model07 = f.getString(row, i++);
            model08 = f.getString(row, i++);
            model09 = f.getString(row, i++);
            model10 = f.getString(row, i++);
            model11 = f.getString(row, i++);
        }
    };
}
