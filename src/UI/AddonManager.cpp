#include "AddonManager.h"
#include "../resource.h"
#include <fstream>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace AddonManager
{
    static bool GetResourceData(int resourceId, const void** outData, DWORD* outSize)
    {
        HRSRC hRes = FindResource(nullptr, MAKEINTRESOURCE(resourceId), RT_RCDATA);
        if (!hRes) return false;
        HGLOBAL hMem = LoadResource(nullptr, hRes);
        if (!hMem) return false;
        *outSize = SizeofResource(nullptr, hRes);
        *outData = LockResource(hMem);
        return *outData != nullptr;
    }

    bool SaveBlenderTerrainAddon(const std::string& outputPath)
    {
        const void* data = nullptr;
        DWORD dataSize = 0;

        if (!GetResourceData(IDR_ADDON_BLENDER_TERRAIN, &data, &dataSize))
        {
            return false;
        }

        std::string fullPath = outputPath;
        if (fullPath.back() != '\\' && fullPath.back() != '/')
        {
            fullPath += "\\";
        }
        fullPath += "wildstar_terrain_importer.zip";

        std::ofstream file(fullPath, std::ios::binary);
        if (!file.is_open())
        {
            return false;
        }

        file.write(static_cast<const char*>(data), dataSize);
        file.close();

        return true;
    }
}