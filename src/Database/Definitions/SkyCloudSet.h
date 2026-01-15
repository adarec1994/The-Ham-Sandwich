#pragma once
#include "../Tbl.h"

namespace Tbl
{
    struct SkyCloudSet
    {
        uint32_t ID;
        float density;
        uint32_t skyTrackCloudSetId00;
        uint32_t skyTrackCloudSetId01;
        uint32_t skyTrackCloudSetId02;
        uint32_t skyTrackCloudSetId03;
        uint32_t skyTrackCloudSetId04;
        uint32_t skyTrackCloudSetId05;

        uint32_t GetID() const { return ID; }

        void Read(const File& f, uint32_t row)
        {
            size_t i = 0;
            ID = f.getUint(row, i++);
            density = f.getFloat(row, i++);
            skyTrackCloudSetId00 = f.getUint(row, i++);
            skyTrackCloudSetId01 = f.getUint(row, i++);
            skyTrackCloudSetId02 = f.getUint(row, i++);
            skyTrackCloudSetId03 = f.getUint(row, i++);
            skyTrackCloudSetId04 = f.getUint(row, i++);
            skyTrackCloudSetId05 = f.getUint(row, i++);
        }
    };
}
