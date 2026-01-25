#include "Sky.h"
#include <cstring>
#include <cstdio>

namespace Sky {

static const size_t HEADER_SIZE = 1216;

static std::wstring scanForModelPath(const uint8_t* data, size_t dataSize, size_t startPos)
{
    std::wstring result;
    size_t pos = startPos;

    while (pos + 1 < dataSize)
    {
        wchar_t ch = data[pos] | (data[pos + 1] << 8);
        pos += 2;
        if (ch == 0) break;
        result += ch;
    }
    return result;
}

static uint32_t readU32At(const uint8_t* data, size_t pos)
{
    return data[pos] | (data[pos+1] << 8) | (data[pos+2] << 16) | (data[pos+3] << 24);
}

static std::vector<std::wstring> findAllModelPaths(const uint8_t* data, size_t dataSize)
{
    std::vector<std::wstring> paths;

    for (size_t pos = HEADER_SIZE; pos < dataSize - 10; pos += 2)
    {
        if (data[pos] == 'A' && data[pos+1] == 0 &&
            data[pos+2] == 'r' && data[pos+3] == 0 &&
            data[pos+4] == 't' && data[pos+5] == 0 &&
            data[pos+6] == '\\' && data[pos+7] == 0)
        {
            std::wstring path = scanForModelPath(data, dataSize, pos);

            if (path.find(L".m3") != std::wstring::npos || path.find(L".M3") != std::wstring::npos)
            {
                bool duplicate = false;
                for (const auto& existing : paths)
                {
                    if (existing == path)
                    {
                        duplicate = true;
                        break;
                    }
                }
                if (!duplicate)
                {
                    paths.push_back(path);
                }
            }
        }
    }

    return paths;
}

bool File::load(const std::vector<uint8_t>& data)
{
    if (data.size() < HEADER_SIZE)
    {
        printf("[Sky] File too small: %zu bytes\n", data.size());
        return false;
    }

    const uint8_t* ptr = data.data();

    uint32_t magic = readU32At(ptr, 0);
    if (magic != 0x58534B59)
    {
        printf("[Sky] Invalid magic: 0x%08X\n", magic);
        return false;
    }

    mVersion = readU32At(ptr, 4);
    printf("[Sky] version=%u, fileSize=%zu\n", mVersion, data.size());

    auto modelPaths = findAllModelPaths(ptr, data.size());
    printf("[Sky] Found %zu model paths\n", modelPaths.size());

    for (const auto& path : modelPaths)
    {
        SkyboxModel model;
        model.modelPath = path;

        AngleAndColor defaultAC;
        defaultAC.angle = glm::vec4(0.0f);
        defaultAC.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        model.angleAndColor.timestamps.push_back(0);
        model.angleAndColor.data.push_back(defaultAC);

        std::string narrow(path.begin(), path.end());
        printf("[Sky] Model: %s\n", narrow.c_str());

        mSkyboxModels.push_back(std::move(model));
    }

    printf("[Sky] Loaded %zu skybox models\n", mSkyboxModels.size());

    return true;
}

glm::vec4 File::getSunColor(uint32_t timeMs) const
{
    if (mSunLightColor.data.empty())
        return glm::vec4(1.0f, 0.95f, 0.9f, 1.0f);
    return mSunLightColor.sample(timeMs);
}

FogSettings File::getFog(uint32_t timeMs) const
{
    return FogSettings{};
}

PostFXSettings File::getPostFX(uint32_t timeMs) const
{
    return PostFXSettings{};
}

}