#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <glm/glm.hpp>

namespace Sky {

struct AngleAndColor
{
    glm::vec4 angle = glm::vec4(0.0f);
    glm::vec4 color = glm::vec4(1.0f);
};

struct FogSettings
{
    float unk0 = 0.0f;
    float fogStartDistance = 1000.0f;
    float unk1 = 0.0f;
    float unk2 = 0.0f;
    float unk3 = 0.0f;
};

struct PostFXSettings
{
    glm::vec4 overlayColor = glm::vec4(0.0f);
    float saturation = 1.0f;
    float brightness = 1.0f;
    float exposure = 1.0f;
    float gamma = 2.2f;
    float bloomAlpha = 0.0f;
    float bloomStrength = 0.0f;
};

template<typename T>
struct TimeTrack
{
    std::vector<uint32_t> timestamps;
    std::vector<T> data;

    T sample(uint32_t timeMs) const
    {
        if (data.empty()) return T{};
        if (timestamps.empty() || timeMs <= timestamps.front()) return data.front();
        if (timeMs >= timestamps.back()) return data.back();

        for (size_t i = 0; i < timestamps.size() - 1; i++)
        {
            if (timeMs >= timestamps[i] && timeMs < timestamps[i + 1])
            {
                return data[i];
            }
        }
        return data.back();
    }

    bool empty() const { return data.empty(); }
};

struct SkyboxModel
{
    int16_t unk0 = 0;
    int16_t unk1 = 0;
    std::wstring modelPath;
    TimeTrack<AngleAndColor> angleAndColor;

    glm::vec4 getColor(uint32_t timeMs) const
    {
        if (angleAndColor.empty())
            return glm::vec4(1.0f);
        return angleAndColor.sample(timeMs).color;
    }

    glm::vec4 getAngle(uint32_t timeMs) const
    {
        if (angleAndColor.empty())
            return glm::vec4(0.0f);
        return angleAndColor.sample(timeMs).angle;
    }
};

class File
{
public:
    File() = default;
    bool load(const std::vector<uint8_t>& data);

    uint32_t getVersion() const { return mVersion; }

    glm::vec4 getSunColor(uint32_t timeMs) const;
    FogSettings getFog(uint32_t timeMs) const;
    PostFXSettings getPostFX(uint32_t timeMs) const;

    const std::vector<SkyboxModel>& getSkyboxModels() const { return mSkyboxModels; }
    const std::wstring& getEnvironmentMap() const { return mEnvironmentMapPath; }
    const std::wstring& getLutFile() const { return mLutFilePath; }

private:
    uint32_t mVersion = 0;
    int32_t mUnk0 = 0;
    int32_t mUnk1 = 0;
    float mUnk2 = 0.0f;

    TimeTrack<glm::vec4> mSunLightColor;
    TimeTrack<AngleAndColor> mSpecularColor;
    TimeTrack<FogSettings> mFogSettings;
    TimeTrack<PostFXSettings> mPostFXSettings;

    std::vector<SkyboxModel> mSkyboxModels;
    std::wstring mEnvironmentMapPath;
    std::wstring mLutFilePath;
};

}