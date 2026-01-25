#include "Sky_Manager.h"
#include "../Area/AreaFile.h"
#include "../Database/Tbl.h"
#include "../models/M3Render.h"
#include "../models/M3Loader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <algorithm>

namespace Sky {

Manager& Manager::Instance()
{
    static Manager instance;
    return instance;
}

bool Manager::loadWorldSkyTable()
{
    if (mTableLoaded) return true;
    if (!mArchive)
    {
        printf("[SkyManager] loadWorldSkyTable: no archive\n");
        return false;
    }

    std::vector<std::wstring> tablePaths = {
        L"DB/WorldSky2.tbl",
        L"DB/WorldSky.tbl"
    };

    for (const auto& tablePath : tablePaths)
    {
        try
        {
            auto entry = mArchive->getFileInfoByPath(tablePath);
            if (!entry) continue;

            std::vector<uint8_t> data;
            if (!mArchive->getFileData(entry, data)) continue;

            Tbl::File table;
            if (!table.load(data.data(), data.size()))
            {
                printf("[SkyManager] Failed to parse %ls\n", tablePath.c_str());
                continue;
            }

            printf("[SkyManager] Loaded %ls with %u records\n", tablePath.c_str(), table.getRecordCount());

            for (uint32_t i = 0; i < table.getRecordCount(); i++)
            {
                uint32_t id = table.getUint(i, 0);
                if (id == 0) continue;

                std::wstring assetPath = table.getString(i, 1);

                std::string path;
                path.reserve(assetPath.size());
                for (wchar_t wc : assetPath)
                {
                    path += static_cast<char>(wc);
                }

                mSkyIDToPath[id] = path;
                mAvailableSkies.push_back(id);
            }

            mTableLoaded = true;
            printf("[SkyManager] Registered %zu sky entries\n", mSkyIDToPath.size());
            return true;
        }
        catch (const std::exception& e)
        {
            printf("[SkyManager] Exception loading %ls: %s\n", tablePath.c_str(), e.what());
        }
        catch (...)
        {
            printf("[SkyManager] Unknown exception loading %ls\n", tablePath.c_str());
        }
    }

    printf("[SkyManager] No WorldSky table found\n");
    return false;
}

void Manager::collectSkyIDsFromArea(const AreaFile* area)
{
    mCollectedSkyIDs.clear();

    if (!area)
    {
        printf("[SkyManager] collectSkyIDsFromArea: area is null\n");
        fflush(stdout);
        return;
    }

    const auto& chunks = area->getChunks();
    printf("[SkyManager] collectSkyIDsFromArea: checking %zu chunks\n", chunks.size());
    fflush(stdout);

    for (size_t idx = 0; idx < chunks.size(); idx++)
    {
        const auto& chunk = chunks[idx];
        if (!chunk) continue;

        const SkyCorner* skyCorners = chunk->getSkyCorners();
        if (!skyCorners) continue;

        for (int corner = 0; corner < 4; corner++)
        {
            for (int i = 0; i < 4; i++)
            {
                uint32_t skyID = skyCorners[corner].worldSkyIDs[i];
                if (skyID != 0)
                {
                    mCollectedSkyIDs.insert(skyID);
                }
            }
        }
    }

    printf("[SkyManager] Collected %zu unique sky IDs from area\n", mCollectedSkyIDs.size());
    fflush(stdout);

    for (uint32_t id : mCollectedSkyIDs)
    {
        auto it = mSkyIDToPath.find(id);
        if (it != mSkyIDToPath.end())
        {
            printf("  Sky ID %u -> %s\n", id, it->second.c_str());
        }
        else
        {
            printf("  Sky ID %u -> (not in table)\n", id);
        }
    }
    fflush(stdout);
}

bool Manager::loadSkyForArea(const AreaFile* area)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mIsLoading = true;

    printf("[SkyManager] loadSkyForArea called\n");
    fflush(stdout);

    if (!area)
    {
        printf("[SkyManager] loadSkyForArea: area is null\n");
        fflush(stdout);
        mIsLoading = false;
        return false;
    }

    if (!mArchive)
    {
        printf("[SkyManager] Getting archive from area...\n");
        fflush(stdout);
        mArchive = area->getArchive();
        if (!mArchive)
        {
            printf("[SkyManager] loadSkyForArea: archive is null\n");
            fflush(stdout);
            mIsLoading = false;
            return false;
        }
    }

    if (!mTableLoaded)
    {
        printf("[SkyManager] Loading WorldSky table...\n");
        fflush(stdout);
        if (!loadWorldSkyTable())
        {
            printf("[SkyManager] loadSkyForArea: failed to load WorldSky table\n");
            fflush(stdout);
        }
    }

    printf("[SkyManager] Collecting sky IDs from area...\n");
    fflush(stdout);
    collectSkyIDsFromArea(area);

    printf("[SkyManager] Loading first available sky...\n");
    fflush(stdout);
    bool result = loadFirstAvailableSky();
    mIsLoading = false;
    return result;
}

bool Manager::loadFirstAvailableSky()
{
    if (!mTableLoaded)
        loadWorldSkyTable();

    for (uint32_t id : mCollectedSkyIDs)
    {
        if (loadSkyFromID(id))
            return true;
    }

    return false;
}

bool Manager::loadSkyFromID(uint32_t skyID)
{
    auto existingIt = mSkyIDToLoadedIndex.find(skyID);
    if (existingIt != mSkyIDToLoadedIndex.end())
    {
        mActiveSkyIndex = existingIt->second;
        printf("[SkyManager] Switched to already-loaded sky ID %u\n", skyID);
        return true;
    }

    auto pathIt = mSkyIDToPath.find(skyID);
    if (pathIt == mSkyIDToPath.end())
    {
        printf("[SkyManager] Sky ID %u not found in table\n", skyID);
        return false;
    }

    if (loadSkyFromPath(pathIt->second))
    {
        mSkyIDToLoadedIndex[skyID] = mActiveSkyIndex;
        return true;
    }

    return false;
}

bool Manager::loadSkyFromPath(const std::string& path)
{
    if (!mArchive) return false;

    std::wstring wpath;
    wpath.reserve(path.size());
    for (char c : path)
    {
        wpath += static_cast<wchar_t>(c);
    }

    auto entry = mArchive->getFileInfoByPath(wpath);
    if (!entry)
    {
        printf("[SkyManager] Sky file not found: %s\n", path.c_str());
        return false;
    }

    std::vector<uint8_t> data;
    if (!mArchive->getFileData(entry, data))
    {
        printf("[SkyManager] Failed to read sky file: %s\n", path.c_str());
        return false;
    }

    auto sky = std::make_unique<File>();
    if (!sky->load(data))
    {
        printf("[SkyManager] Failed to parse sky file: %s\n", path.c_str());
        return false;
    }

    mLoadedSkies.push_back(std::move(sky));
    mActiveSkyIndex = static_cast<int>(mLoadedSkies.size()) - 1;

    printf("[SkyManager] Loaded sky: %s (version %u)\n", path.c_str(), mLoadedSkies.back()->getVersion());

    const auto& models = mLoadedSkies.back()->getSkyboxModels();
    printf("[SkyManager] Sky has %zu skybox models\n", models.size());

    loadSkyboxModels();

    return true;
}

void Manager::loadSkyboxModels()
{
    if (!mArchive)
    {
        printf("[SkyManager] loadSkyboxModels: missing archive\n");
        return;
    }

    const File* sky = getActiveSky();
    if (!sky) return;

    const auto& models = sky->getSkyboxModels();

    for (const auto& model : models)
    {
        if (model.modelPath.empty()) continue;

        if (mLoadedModelPaths.count(model.modelPath) > 0)
        {
            continue;
        }

        auto entry = mArchive->getFileInfoByPath(model.modelPath);
        if (!entry)
        {
            std::string narrow(model.modelPath.begin(), model.modelPath.end());
            printf("[SkyManager] Model not found: %s\n", narrow.c_str());
            continue;
        }

        std::vector<uint8_t> data;
        if (!mArchive->getFileData(entry, data))
        {
            std::string narrow(model.modelPath.begin(), model.modelPath.end());
            printf("[SkyManager] Failed to read model: %s\n", narrow.c_str());
            continue;
        }

        M3ModelData modelData = M3Loader::Load(data);
        if (!modelData.success)
        {
            std::string narrow(model.modelPath.begin(), model.modelPath.end());
            printf("[SkyManager] Failed to parse M3: %s\n", narrow.c_str());
            continue;
        }

        auto m3 = std::make_unique<M3Render>(modelData, mArchive, false, false);

        std::string narrow(model.modelPath.begin(), model.modelPath.end());
        printf("[SkyManager] Loaded skybox M3: %s (%zu textures, %zu submeshes)\n",
               narrow.c_str(), modelData.textures.size(), modelData.geometry.submeshes.size());

        mLoadedModelPaths.insert(model.modelPath);
        mSkyModelPathsOrdered.push_back(model.modelPath);
        mSkyboxM3s.push_back(std::move(m3));
    }

    printf("[SkyManager] Loaded %zu unique skybox M3 models\n", mSkyboxM3s.size());
}

M3Render* Manager::getSelectedSkyModel()
{
    std::lock_guard<std::mutex> lock(mMutex);
    if (mSelectedSkyModelIndex < 0 || mSelectedSkyModelIndex >= static_cast<int>(mSkyboxM3s.size()))
        return nullptr;
    return mSkyboxM3s[mSelectedSkyModelIndex].get();
}

size_t Manager::getSkyboxM3Count() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mSkyboxM3s.size();
}

std::vector<std::wstring> Manager::getSkyModelPaths() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mSkyModelPathsOrdered;
}

M3Render* Manager::getSkyboxM3(size_t index)
{
    std::lock_guard<std::mutex> lock(mMutex);
    if (index >= mSkyboxM3s.size())
        return nullptr;
    return mSkyboxM3s[index].get();
}

void Manager::render(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& cameraPos)
{
    std::lock_guard<std::mutex> lock(mMutex);

    if (mSkyboxM3s.empty()) return;

    for (auto& m3 : mSkyboxM3s)
    {
        if (m3 && m3->hasPendingTextures())
        {
            m3->uploadNextTexture(mArchive);
        }
    }

    glm::mat4 skyModel = glm::translate(glm::mat4(1.0f), cameraPos);

    const File* activeSky = nullptr;
    if (mActiveSkyIndex >= 0 && mActiveSkyIndex < static_cast<int>(mLoadedSkies.size()))
    {
        activeSky = mLoadedSkies[mActiveSkyIndex].get();
    }

    for (size_t i = 0; i < mSkyboxM3s.size(); i++)
    {
        auto& m3 = mSkyboxM3s[i];
        if (!m3 || m3->getSubmeshCount() == 0) continue;

        bool isSelected = (static_cast<int>(i) == mSelectedSkyModelIndex);

        if (isSelected)
        {
            m3->setHighlightColor(0.2f, 1.0f, 0.2f, 0.3f);
        }
        else if (activeSky && i < activeSky->getSkyboxModels().size())
        {
            glm::vec4 color = activeSky->getSkyboxModels()[i].getColor(mTimeOfDayMs);
            m3->setHighlightColor(color.r, color.g, color.b, color.a * 0.5f);
        }

        m3->renderGlm(view, proj, skyModel);

        m3->setHighlightColor(0.0f, 0.0f, 0.0f, 0.0f);
    }
}

const File* Manager::getActiveSky() const
{
    if (mActiveSkyIndex < 0 || mActiveSkyIndex >= static_cast<int>(mLoadedSkies.size()))
        return nullptr;
    return mLoadedSkies[mActiveSkyIndex].get();
}

glm::vec4 Manager::getCurrentSunColor() const
{
    const File* sky = getActiveSky();
    if (!sky) return glm::vec4(1.0f);
    return sky->getSunColor(mTimeOfDayMs);
}

FogSettings Manager::getCurrentFog() const
{
    const File* sky = getActiveSky();
    if (!sky) return FogSettings{};
    return sky->getFog(mTimeOfDayMs);
}

PostFXSettings Manager::getCurrentPostFX() const
{
    const File* sky = getActiveSky();
    if (!sky) return PostFXSettings{};
    return sky->getPostFX(mTimeOfDayMs);
}

}