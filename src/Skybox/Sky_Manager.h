#pragma once
#include "Sky.h"
#include "../Archive.h"
#include <memory>
#include <map>
#include <set>
#include <string>
#include <mutex>
#include <atomic>

class AreaFile;
class M3Render;

namespace Sky {

class Manager
{
public:
    static Manager& Instance();

    void setArchive(std::shared_ptr<Archive> arc) { mArchive = arc; }

    bool loadWorldSkyTable();

    void collectSkyIDsFromArea(const AreaFile* area);

    bool loadSkyForArea(const AreaFile* area);

    bool loadSkyFromID(uint32_t skyID);

    bool loadSkyFromPath(const std::string& path);

    bool loadFirstAvailableSky();

    void loadSkyboxModels();

    void render(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& cameraPos);

    bool hasSky() const { return mActiveSkyIndex >= 0; }

    const File* getActiveSky() const;

    uint32_t getTimeOfDayMs() const { return mTimeOfDayMs; }
    void setTimeOfDayMs(uint32_t ms) { mTimeOfDayMs = ms; }

    float getTimeOfDayHours() const { return static_cast<float>(mTimeOfDayMs) / 3600000.0f; }
    void setTimeOfDayHours(float hours) { mTimeOfDayMs = static_cast<uint32_t>(hours * 3600000.0f); }

    glm::vec4 getCurrentSunColor() const;
    FogSettings getCurrentFog() const;
    PostFXSettings getCurrentPostFX() const;

    const std::map<uint32_t, std::string>& getSkyIDToPath() const { return mSkyIDToPath; }
    const std::set<uint32_t>& getCollectedSkyIDs() const { return mCollectedSkyIDs; }
    const std::vector<uint32_t>& getAvailableSkies() const { return mAvailableSkies; }

    int getActiveSkyIndex() const { return mActiveSkyIndex; }

    size_t getSkyboxM3Count() const;
    M3Render* getSkyboxM3(size_t index);
    const std::vector<std::unique_ptr<M3Render>>& getSkyboxM3s() const { return mSkyboxM3s; }

    int getSelectedSkyModelIndex() const { return mSelectedSkyModelIndex; }
    void setSelectedSkyModelIndex(int idx) { mSelectedSkyModelIndex = idx; }
    M3Render* getSelectedSkyModel();

    void hideSkyModel(int index);
    void showAllHiddenSkyModels();
    void deleteSkyModel(int index);
    bool isSkyModelHidden(int index) const;
    bool isSkyModelDeleted(int index) const;
    bool isSkyModelVisible(int index) const;

    std::vector<std::wstring> getSkyModelPaths() const;

    bool isLoading() const { return mIsLoading; }

private:
    Manager() = default;
    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;

    mutable std::mutex mMutex;
    std::shared_ptr<Archive> mArchive;

    bool mTableLoaded = false;
    std::map<uint32_t, std::string> mSkyIDToPath;
    std::vector<uint32_t> mAvailableSkies;

    std::set<uint32_t> mCollectedSkyIDs;

    std::vector<std::unique_ptr<File>> mLoadedSkies;
    std::map<uint32_t, int> mSkyIDToLoadedIndex;
    int mActiveSkyIndex = -1;

    std::vector<std::unique_ptr<M3Render>> mSkyboxM3s;
    std::set<std::wstring> mLoadedModelPaths;
    std::vector<std::wstring> mSkyModelPathsOrdered;
    int mSelectedSkyModelIndex = -1;
    std::set<int> mHiddenSkyModels;
    std::set<int> mDeletedSkyModels;
    std::atomic<bool> mIsLoading{false};

    uint32_t mTimeOfDayMs = 12 * 3600000;
};

}