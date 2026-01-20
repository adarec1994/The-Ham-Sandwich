#include "AreaFile.h"
#include "TerrainTexture.h"
#include "Props.h"
#include "../models/M3Loader.h"
#include "../models/M3Render.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <algorithm>
#include <string>
#include <memory>
#include <cstring>
#include <limits>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <thread>
#include <chrono>

static std::string NormalizePropPath(const std::string& path)
{
    std::string result = path;
    for (char& c : result)
        if (c == '\\') c = '/';
    while (!result.empty() && result[0] == '/')
        result.erase(0, 1);
    return result;
}

const float AreaFile::UnitSize = 2.0f;
const float AreaFile::GRID_SIZE = 512.0f;

static int gReferenceTileX = -1;
static int gReferenceTileY = -1;

std::vector<uint32> AreaChunkRender::indices;
AreaChunkRender::Uniforms AreaChunkRender::uniforms;

const AreaChunkRender::Uniforms& AreaChunkRender::getUniforms() { return uniforms; }

void ResetAreaReferencePosition() { gReferenceTileX = -1; gReferenceTileY = -1; }

static inline int hexNibble(wchar_t c)
{
    if (c >= L'0' && c <= L'9') return static_cast<int>(c - L'0');
    if (c >= L'a' && c <= L'f') return 10 + static_cast<int>(c - L'a');
    if (c >= L'A' && c <= L'F') return 10 + static_cast<int>(c - L'A');
    return -1;
}

static inline bool parseHexByte(const std::wstring& s, size_t pos, int& outByte)
{
    if (pos + 2 > s.size()) return false;
    int hi = hexNibble(s[pos]);
    int lo = hexNibble(s[pos + 1]);
    if (hi < 0 || lo < 0) return false;
    outByte = (hi << 4) | lo;
    return true;
}

static GLuint gFallbackWhite = 0;
static GLuint gFallbackNormal = 0;
static uint32 gLastTerrainProgram = 0;

static void EnsureFallbackTextures()
{
    if (gFallbackWhite == 0)
    {
        uint8_t px[4] = { 255, 255, 255, 255 };
        glGenTextures(1, &gFallbackWhite);
        glBindTexture(GL_TEXTURE_2D, gFallbackWhite);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    }
    if (gFallbackNormal == 0)
    {
        uint8_t px[4] = { 128, 128, 255, 255 };
        glGenTextures(1, &gFallbackNormal);
        glBindTexture(GL_TEXTURE_2D, gFallbackNormal);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    }
}

void AreaFile::parseTileXYFromFilename()
{
    if (mPath.empty()) return;
    std::wstring name = mPath;
    size_t ext = name.rfind(L".area");
    if (ext == std::wstring::npos || ext < 5) return;
    size_t dot = ext - 5;
    if (name[dot] != L'.') return;
    int bx = 0, by = 0;
    if (!parseHexByte(name, dot + 1, bx)) return;
    if (!parseHexByte(name, dot + 3, by)) return;
    mTileX = bx;
    mTileY = by;
}

void AreaFile::calculateWorldOffset()
{
    if (gReferenceTileX < 0 || gReferenceTileY < 0)
    {
        gReferenceTileX = mTileX;
        gReferenceTileY = mTileY;
    }
    mWorldOffset.x = static_cast<float>(mTileX - gReferenceTileX) * GRID_SIZE;
    mWorldOffset.y = 0.0f;
    mWorldOffset.z = static_cast<float>(mTileY - gReferenceTileY) * GRID_SIZE;
}

AreaFile::AreaFile(ArchivePtr archive, FileEntryPtr file)
    : mArchive(std::move(archive)), mFile(std::move(file))
    , mMinBounds(std::numeric_limits<float>::max())
    , mMaxBounds(std::numeric_limits<float>::lowest())
{
    mBaseColor = glm::vec4(1.0f);
    if (mArchive && mFile)
    {
        mPath = mFile->getFullPath();
        try
        {
            mArchive->getFileData(mFile, mContent);
            mStream = std::make_shared<BinStream>(mContent);
            parseTileXYFromFilename();
            calculateWorldOffset();
        }
        catch (...) {}
    }
}

AreaFile::~AreaFile()
{
    if (mTextureID != 0) glDeleteTextures(1, &mTextureID);
}

bool AreaFile::loadTexture() { return false; }

bool AreaFile::load()
{
    if (mContent.size() < 8) return false;
    struct R
    {
        const uint8* p; const uint8* e;
        bool can(size_t n) const { return static_cast<size_t>(e - p) >= n; }
        uint32 u32le()
        {
            uint32 v = static_cast<uint32>(p[0]) | (static_cast<uint32>(p[1]) << 8) |
                      (static_cast<uint32>(p[2]) << 16) | (static_cast<uint32>(p[3]) << 24);
            p += 4; return v;
        }
        void bytes(void* out, size_t n) { memcpy(out, p, n); p += n; }
        void skip(size_t n) { p += n; }
    };
    const uint8* base = mContent.data();
    R r{ base, base + mContent.size() };
    uint32 sig = r.u32le();
    if (sig != AreaChunkID::area && sig != AreaChunkID::AREA) return false;
    r.u32le();
    std::vector<uint8> chnkData, propData, curtData;
    while (r.can(8))
    {
        uint32 magic = r.u32le();
        uint32 size = r.u32le();
        if (!r.can(size)) break;
        switch (magic)
        {
            case AreaChunkID::CHNK: chnkData.resize(size); r.bytes(chnkData.data(), size); break;
            case AreaChunkID::PROp: propData.resize(size); r.bytes(propData.data(), size); break;
            case AreaChunkID::CURT: curtData.resize(size); r.bytes(curtData.data(), size); break;
            default: r.skip(size); break;
        }
    }

    if (!propData.empty()) ParsePropsChunk(propData.data(), propData.size(), mProps, mPropLookup);
    if (!curtData.empty()) ParseCurtsChunk(curtData.data(), curtData.size(), mCurts);
    if (chnkData.empty()) { mMinBounds = glm::vec3(0, 0, 0); mMaxBounds = glm::vec3(512, 50, 512); return true; }

    struct ChunkJob
    {
        uint32 index;
        uint32 cellX;
        uint32 cellY;
        std::vector<uint8> data;
    };
    std::vector<ChunkJob> jobs;
    jobs.reserve(256);

    R cr{ chnkData.data(), chnkData.data() + chnkData.size() };
    uint32 lastIndex = 0;
    while (cr.can(4))
    {
        uint32 cellInfo = cr.u32le();
        uint32 idxDelta = (cellInfo >> 24) & 0xFF;
        uint32 size = cellInfo & 0x00FFFFFF;
        uint32 index = idxDelta + lastIndex;
        lastIndex = index + 1;
        if (index >= 256) break;
        if (!cr.can(size)) break;
        if (size < 4) { cr.skip(size); continue; }
        ChunkJob job;
        job.index = index;
        job.cellX = index % 16;
        job.cellY = index / 16;
        job.data.resize(size);
        cr.bytes(job.data.data(), size);
        jobs.push_back(std::move(job));
    }

    std::vector<ParsedChunk> parsedChunks(256);
    size_t numThreads = std::max(1u, std::thread::hardware_concurrency());

    std::vector<std::thread> threads;
    std::atomic<size_t> jobIndex{0};

    for (size_t t = 0; t < numThreads; t++)
    {
        threads.emplace_back([&]()
        {
            while (true)
            {
                size_t i = jobIndex.fetch_add(1);
                if (i >= jobs.size()) break;
                auto& job = jobs[i];
                parsedChunks[job.index] = AreaChunkRender::parseChunkData(job.data, job.cellX, job.cellY);
            }
        });
    }
    for (auto& t : threads) t.join();

    mChunks.assign(256, nullptr);
    uint32 validCount = 0;
    float totalH = 0.0f;
    for (size_t i = 0; i < 256; i++)
    {
        if (!parsedChunks[i].valid) continue;
        auto chunk = std::make_shared<AreaChunkRender>(std::move(parsedChunks[i]), mArchive);
        mChunks[i] = chunk;
        if (chunk && chunk->isFullyInitialized())
        {
            totalH += chunk->getAverageHeight();
            validCount++;
            mMaxHeight = std::max(mMaxHeight, chunk->getMaxHeight());
            mMinBounds = glm::min(mMinBounds, chunk->getMinBounds());
            mMaxBounds = glm::max(mMaxBounds, chunk->getMaxBounds());
        }
    }

    if (validCount > 0) mAverageHeight = totalH / static_cast<float>(validCount);
    else { mMinBounds = glm::vec3(0, 0, 0); mMaxBounds = glm::vec3(512, 50, 512); }
    return true;
}

bool AreaFile::loadFromParsed(ParsedArea&& parsed)
{
    mPath = parsed.path;
    mTileX = parsed.tileX;
    mTileY = parsed.tileY;
    calculateWorldOffset();
    mProps = std::move(parsed.props);
    mCurts = std::move(parsed.curts);
    mPropLookup.clear();
    for (size_t i = 0; i < mProps.size(); i++) mPropLookup[mProps[i].uniqueID] = i;
    mChunks.assign(256, nullptr);
    float totalH = 0.0f; uint32 validCount = 0;
    for (size_t i = 0; i < parsed.chunks.size() && i < 256; i++)
    {
        if (!parsed.chunks[i].valid) continue;
        auto chunk = std::make_shared<AreaChunkRender>(std::move(parsed.chunks[i]), mArchive);
        mChunks[i] = chunk;
        if (chunk && chunk->isFullyInitialized())
        {
            totalH += chunk->getAverageHeight();
            validCount++;
            mMaxHeight = std::max(mMaxHeight, chunk->getMaxHeight());
            mMinBounds = glm::min(mMinBounds, chunk->getMinBounds());
            mMaxBounds = glm::max(mMaxBounds, chunk->getMaxBounds());
        }
    }
    if (validCount > 0) mAverageHeight = totalH / static_cast<float>(validCount);
    else { mMinBounds = glm::vec3(0, 0, 0); mMaxBounds = glm::vec3(512, 50, 512); }
    return true;
}

void AreaFile::render(const Matrix& matView, const Matrix& matProj, uint32 shaderProgram, const AreaChunkRenderPtr& selectedChunk)
{
    glUseProgram(shaderProgram);
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    if (viewLoc != -1) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &matView[0][0]);
    if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, &matProj[0][0]);
    EnsureFallbackTextures();
    if (gLastTerrainProgram != shaderProgram) { AreaChunkRender::geometryInit(shaderProgram); gLastTerrainProgram = shaderProgram; }
    glm::mat4 worldModel(1.0f);
    worldModel = glm::translate(worldModel, mWorldOffset);
    worldModel = glm::rotate(worldModel, glm::radians(mGlobalRotation), glm::vec3(0.0f, 1.0f, 0.0f));
    const auto& u = AreaChunkRender::getUniforms();
    if (u.model != static_cast<uint32>(-1)) glUniformMatrix4fv(u.model, 1, GL_FALSE, &worldModel[0][0]);
    if (u.baseColor != static_cast<uint32>(-1)) glUniform4f(u.baseColor, 1.0f, 1.0f, 1.0f, 1.0f);
    for (auto& c : mChunks)
    {
        if (!c || !c->isFullyInitialized()) continue;
        c->loadTextures(mArchive);
        GLint highlightLoc = glGetUniformLocation(shaderProgram, "highlightColor");
        if (highlightLoc != -1)
        {
            if (c == selectedChunk) glUniform4f(highlightLoc, 1.0f, 1.0f, 0.0f, 0.5f);
            else glUniform4f(highlightLoc, 0.0f, 0.0f, 0.0f, 0.0f);
        }
        c->bindTextures(shaderProgram);
        c->render();
    }
}

bool AreaFile::loadProp(uint32_t uniqueID)
{
    auto it = mPropLookup.find(uniqueID);
    if (it == mPropLookup.end() || it->second >= mProps.size()) return false;
    Prop& prop = mProps[it->second];
    if (prop.loaded) return true;
    if (prop.loadRequested) return false;
    PropLoader::Instance().SetArchive(mArchive);
    PropLoader::Instance().QueueProp(&prop);
    return true;
}

void AreaFile::loadAllProps()
{
    loadAllPropsAsync();
    while (PropLoader::Instance().HasPendingWork())
    {
        PropLoader::Instance().ProcessGPUUploads(200);
    }
    PropLoader::Instance().ProcessGPUUploads(500);
}

void AreaFile::loadAllPropsWithProgress(std::function<void(size_t, size_t)> progressCallback)
{
    size_t total = 0;
    for (auto& prop : mProps)
    {
        if (!prop.loaded && !prop.loadRequested)
            total++;
    }

    if (total == 0)
    {
        if (progressCallback) progressCallback(0, 0);
        return;
    }

    loadAllPropsAsync();

    while (PropLoader::Instance().HasPendingWork())
    {
        PropLoader::Instance().ProcessGPUUploads(100);

        size_t loaded = 0;
        for (auto& prop : mProps)
        {
            if (prop.loaded) loaded++;
        }

        if (progressCallback)
            progressCallback(loaded, total);
    }

    PropLoader::Instance().ProcessGPUUploads(500);

    if (progressCallback)
        progressCallback(total, total);
}

void AreaFile::loadAllPropsAsync()
{
    auto& loader = PropLoader::Instance();
    loader.SetArchive(mArchive);
    std::vector<Prop*> toLoad;
    toLoad.reserve(mProps.size());
    for (auto& prop : mProps)
    {
        if (!prop.loaded && !prop.loadRequested)
            toLoad.push_back(&prop);
    }
    if (!toLoad.empty())
        loader.QueueProps(toLoad);
}

void AreaFile::loadPropsInView(const glm::vec3& cameraPos, float radius)
{
    auto& loader = PropLoader::Instance();
    loader.SetArchive(mArchive);
    std::vector<std::pair<float, Prop*>> sorted;
    sorted.reserve(mProps.size());
    float radiusSq = radius * radius;
    for (auto& prop : mProps)
    {
        if (prop.loaded || prop.loadRequested) continue;
        glm::vec3 propWorldPos = prop.position + mWorldOffset;
        float distSq = glm::dot(propWorldPos - cameraPos, propWorldPos - cameraPos);
        if (distSq <= radiusSq)
            sorted.push_back({distSq, &prop});
    }
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    std::vector<Prop*> toLoad;
    toLoad.reserve(sorted.size());
    for (auto& [dist, prop] : sorted)
        toLoad.push_back(prop);
    if (!toLoad.empty())
        loader.QueueProps(toLoad);
}

void AreaFile::updatePropLoading()
{
    PropLoader::Instance().ProcessGPUUploads(10);
}

void AreaFile::renderProps(const Matrix& matView, const Matrix& matProj)
{
    static bool debugPrinted = false;
    int rendered = 0;

    // Print debug info once
    if (!debugPrinted && !mProps.empty())
    {
        std::cout << "[PropDebug] ===== COORDINATE ANALYSIS =====" << std::endl;
        std::cout << "[PropDebug] Tile from filename: " << mTileX << ", " << mTileY << std::endl;
        std::cout << "[PropDebug] World offset: " << mWorldOffset.x << ", " << mWorldOffset.y << ", " << mWorldOffset.z << std::endl;
        std::cout << "[PropDebug] Terrain bounds: (" << mMinBounds.x << ", " << mMinBounds.y << ", " << mMinBounds.z << ") to ("
                  << mMaxBounds.x << ", " << mMaxBounds.y << ", " << mMaxBounds.z << ")" << std::endl;

        // Analyze prop positions
        float minPropX = std::numeric_limits<float>::max();
        float maxPropX = std::numeric_limits<float>::lowest();
        float minPropZ = std::numeric_limits<float>::max();
        float maxPropZ = std::numeric_limits<float>::lowest();
        float minPropY = std::numeric_limits<float>::max();
        float maxPropY = std::numeric_limits<float>::lowest();

        for (const auto& prop : mProps)
        {
            minPropX = std::min(minPropX, prop.position.x);
            maxPropX = std::max(maxPropX, prop.position.x);
            minPropY = std::min(minPropY, prop.position.y);
            maxPropY = std::max(maxPropY, prop.position.y);
            minPropZ = std::min(minPropZ, prop.position.z);
            maxPropZ = std::max(maxPropZ, prop.position.z);
        }

        std::cout << "[PropDebug] Prop position ranges:" << std::endl;
        std::cout << "[PropDebug]   X: " << minPropX << " to " << maxPropX << " (span: " << (maxPropX - minPropX) << ")" << std::endl;
        std::cout << "[PropDebug]   Y: " << minPropY << " to " << maxPropY << " (span: " << (maxPropY - minPropY) << ")" << std::endl;
        std::cout << "[PropDebug]   Z: " << minPropZ << " to " << maxPropZ << " (span: " << (maxPropZ - minPropZ) << ")" << std::endl;

        // Show first few props
        int shown = 0;
        for (const auto& prop : mProps)
        {
            if (shown >= 5) break;
            std::cout << "[PropDebug] Prop[" << shown << "]: pos=(" << prop.position.x << ", " << prop.position.y << ", " << prop.position.z
                      << ") scale=" << prop.scale << " path=" << prop.path << std::endl;
            shown++;
        }

        debugPrinted = true;
    }

    static int debugCount = 0;

    for (const auto& prop : mProps)
    {
        if (!prop.loaded || !prop.render) continue;

        // Direct positioning - terrain now uses normal coordinates
        glm::vec3 pos = prop.position;

        // Add world offset for multi-tile support
        pos += mWorldOffset;

        // Debug: print first few with rotation info
        if (debugCount < 5 && !prop.path.empty())
        {
            std::cout << "[PropRender] " << prop.path.substr(prop.path.rfind('\\') + 1)
                      << " pos=(" << pos.x << ", " << pos.y << ", " << pos.z << ")"
                      << " scale=" << prop.scale << std::endl;
            debugCount++;
        }

        // Build model matrix
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, pos);
        model = model * glm::mat4_cast(prop.rotation);
        model = glm::scale(model, glm::vec3(prop.scale));

        if (mGlobalRotation != 0.0f)
        {
            glm::mat4 areaRotation = glm::rotate(glm::mat4(1.0f), glm::radians(mGlobalRotation), glm::vec3(0.0f, 1.0f, 0.0f));
            model = areaRotation * model;
        }

        prop.render->render(matView, matProj, model);
        rendered++;
    }

    // Print render count once
    static bool countPrinted = false;
    if (!countPrinted && rendered > 0)
    {
        std::cout << "[PropDebug] Rendered " << rendered << " props" << std::endl;
        countPrinted = true;
    }
}

const Prop* AreaFile::getPropByID(uint32_t uniqueID) const
{
    auto it = mPropLookup.find(uniqueID);
    if (it != mPropLookup.end() && it->second < mProps.size()) return &mProps[it->second];
    return nullptr;
}

size_t AreaFile::getLoadedPropCount() const
{
    size_t count = 0;
    for (const auto& prop : mProps) if (prop.loaded && prop.render) count++;
    return count;
}

ParsedArea AreaFile::parseAreaFile(const ArchivePtr& archive, const FileEntryPtr& file)
{
    ParsedArea result;
    result.file = file;
    if (!archive || !file) return result;
    result.path = file->getFullPath();
    size_t ext = result.path.rfind(L".area");
    if (ext != std::wstring::npos && ext >= 5)
    {
        size_t dot = ext - 5;
        if (result.path[dot] == L'.')
        {
            int bx = 0, by = 0;
            if (parseHexByte(result.path, dot + 1, bx) && parseHexByte(result.path, dot + 3, by))
            {
                result.tileX = bx;
                result.tileY = by;
            }
        }
    }
    std::vector<uint8_t> content;
    if (!archive->getFileData(file, content) || content.size() < 8) return result;
    const uint8* base = content.data();
    const uint8* end = base + content.size();
    const uint8* ptr = base;
    auto canRead = [&](size_t n) { return static_cast<size_t>(end - ptr) >= n; };
    auto readU32 = [&]() -> uint32
    {
        uint32 v = static_cast<uint32>(ptr[0]) | (static_cast<uint32>(ptr[1]) << 8) |
                  (static_cast<uint32>(ptr[2]) << 16) | (static_cast<uint32>(ptr[3]) << 24);
        ptr += 4;
        return v;
    };
    uint32 sig = readU32();
    if (sig != AreaChunkID::area && sig != AreaChunkID::AREA) return result;
    readU32();
    std::vector<uint8> chnkData, propData, curtData;
    while (canRead(8))
    {
        uint32 magic = readU32();
        uint32 size = readU32();
        if (!canRead(size)) break;
        switch (magic)
        {
            case AreaChunkID::CHNK: chnkData.resize(size); memcpy(chnkData.data(), ptr, size); break;
            case AreaChunkID::PROp: propData.resize(size); memcpy(propData.data(), ptr, size); break;
            case AreaChunkID::CURT: curtData.resize(size); memcpy(curtData.data(), ptr, size); break;
        }
        ptr += size;
    }
    if (!propData.empty())
    {
        std::unordered_map<uint32_t, size_t> lookup;
        ParsePropsChunk(propData.data(), propData.size(), result.props, lookup);
    }
    if (!curtData.empty()) ParseCurtsChunk(curtData.data(), curtData.size(), result.curts);
    if (chnkData.empty())
    {
        result.valid = true;
        result.minBounds = glm::vec3(0, 0, 0);
        result.maxBounds = glm::vec3(512, 50, 512);
        return result;
    }
    result.chunks.resize(256);
    const uint8* cptr = chnkData.data();
    const uint8* cend = cptr + chnkData.size();
    uint32 lastIndex = 0;
    float totalH = 0.0f;
    uint32 validCount = 0;
    while (static_cast<size_t>(cend - cptr) >= 4)
    {
        uint32 cellInfo = static_cast<uint32>(cptr[0]) | (static_cast<uint32>(cptr[1]) << 8) |
                         (static_cast<uint32>(cptr[2]) << 16) | (static_cast<uint32>(cptr[3]) << 24);
        cptr += 4;
        uint32 idxDelta = (cellInfo >> 24) & 0xFF;
        uint32 size = cellInfo & 0x00FFFFFF;
        uint32 index = idxDelta + lastIndex;
        lastIndex = index + 1;
        if (index >= 256) break;
        if (static_cast<size_t>(cend - cptr) < size) break;
        if (size < 4) { cptr += size; continue; }
        std::vector<uint8> cellData(size);
        memcpy(cellData.data(), cptr, size);
        cptr += size;
        uint32 cellX = index % 16;
        uint32 cellY = index / 16;
        auto chunk = AreaChunkRender::parseChunkData(cellData, cellX, cellY);
        if (chunk.valid)
        {
            totalH += chunk.avgHeight;
            validCount++;
            result.maxHeight = std::max(result.maxHeight, chunk.maxHeight);
            result.minBounds = glm::min(result.minBounds, chunk.minBounds);
            result.maxBounds = glm::max(result.maxBounds, chunk.maxBounds);
        }
        result.chunks[index] = std::move(chunk);
    }
    if (validCount > 0) result.avgHeight = totalH / static_cast<float>(validCount);
    else { result.minBounds = glm::vec3(0, 0, 0); result.maxBounds = glm::vec3(512, 50, 512); }
    result.valid = true;
    return result;
}

void AreaChunkRender::loadTextures(const ArchivePtr& archive)
{
    if (mTexturesLoaded) return;
    auto& texMgr = TerrainTexture::Manager::Instance();
    texMgr.LoadWorldLayerTable(archive);
    if (!mBlendMap.empty()) mBlendMapTexture = texMgr.CreateBlendMapTexture(mBlendMap.data(), 65, 65);
    else if (!mBlendMapDXT.empty()) mBlendMapTexture = texMgr.CreateBlendMapFromDXT1(mBlendMapDXT.data(), mBlendMapDXT.size(), 65, 65);
    else { uint8_t defaultBlend[4] = {255, 0, 0, 0}; mBlendMapTexture = texMgr.CreateBlendMapTexture(defaultBlend, 1, 1); }
    if (!mColorMap.empty()) mColorMapTextureGPU = texMgr.CreateColorMapTexture(mColorMap.data(), 65, 65);
    else if (!mColorMapDXT.empty()) mColorMapTextureGPU = texMgr.CreateColorMapFromDXT5(mColorMapDXT.data(), mColorMapDXT.size(), 65, 65);
    for (int i = 0; i < 4; ++i)
    {
        uint32_t layerId = mWorldLayerIDs[i];
        if (layerId == 0) { mLayerDiffuse[i] = gFallbackWhite; mLayerNormal[i] = gFallbackNormal; mLayerScale[i] = 4.0f; continue; }
        const auto* cached = texMgr.GetLayerTexture(archive, layerId);
        if (cached && cached->loaded) { mLayerDiffuse[i] = cached->diffuse; mLayerNormal[i] = cached->normal ? cached->normal : gFallbackNormal; }
        else { mLayerDiffuse[i] = gFallbackWhite; mLayerNormal[i] = gFallbackNormal; }
        const auto* layerEntry = texMgr.GetLayerEntry(layerId);
        if (layerEntry && layerEntry->scaleU > 0.0f) mLayerScale[i] = layerEntry->scaleU;
        else mLayerScale[i] = 4.0f;
    }
    mTexturesLoaded = true;
}

void AreaChunkRender::bindTextures(unsigned int program) const
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mBlendMapTexture ? mBlendMapTexture : gFallbackWhite);
    glUniform1i(glGetUniformLocation(program, "blendMap"), 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mColorMapTextureGPU ? mColorMapTextureGPU : gFallbackWhite);
    glUniform1i(glGetUniformLocation(program, "colorMap"), 1);
    glUniform1i(glGetUniformLocation(program, "hasColorMap"), mColorMapTextureGPU ? 1 : 0);
    for (int i = 0; i < 4; ++i)
    {
        glActiveTexture(GL_TEXTURE2 + i);
        glBindTexture(GL_TEXTURE_2D, mLayerDiffuse[i] ? mLayerDiffuse[i] : gFallbackWhite);
        const char* names[] = {"layer0", "layer1", "layer2", "layer3"};
        glUniform1i(glGetUniformLocation(program, names[i]), 2 + i);
    }
    for (int i = 0; i < 4; ++i)
    {
        glActiveTexture(GL_TEXTURE6 + i);
        glBindTexture(GL_TEXTURE_2D, mLayerNormal[i] ? mLayerNormal[i] : gFallbackNormal);
        const char* names[] = {"layer0Normal", "layer1Normal", "layer2Normal", "layer3Normal"};
        glUniform1i(glGetUniformLocation(program, names[i]), 6 + i);
    }
    float s0 = mLayerScale[0] > 0.0f ? 32.0f / mLayerScale[0] : 8.0f;
    float s1 = mLayerScale[1] > 0.0f ? 32.0f / mLayerScale[1] : 8.0f;
    float s2 = mLayerScale[2] > 0.0f ? 32.0f / mLayerScale[2] : 8.0f;
    float s3 = mLayerScale[3] > 0.0f ? 32.0f / mLayerScale[3] : 8.0f;
    glUniform4f(glGetUniformLocation(program, "texScale"), s0, s1, s2, s3);
    glActiveTexture(GL_TEXTURE0);
}

ParsedChunk AreaChunkRender::parseChunkData(const std::vector<uint8>& cellData, uint32 cellX, uint32 cellY)
{
    ParsedChunk result;
    result.cellX = cellX;
    result.cellY = cellY;
    if (cellData.size() < 4) return result;
    struct R
    {
        const uint8* p; const uint8* e;
        bool can(size_t n) const { return static_cast<size_t>(e - p) >= n; }
        uint32 u32le() { uint32 v = static_cast<uint32>(p[0]) | (static_cast<uint32>(p[1]) << 8) | (static_cast<uint32>(p[2]) << 16) | (static_cast<uint32>(p[3]) << 24); p += 4; return v; }
        uint16 u16le() { uint16 v = static_cast<uint16>(p[0]) | (static_cast<uint16>(p[1]) << 8); p += 2; return v; }
        uint8 u8() { return *p++; }
        void skip(size_t n) { p += n; }
    };
    R r{ cellData.data(), cellData.data() + cellData.size() };
    result.flags = r.u32le();
    std::vector<uint16> heightmap;
    bool hasHeightmap = false;
    if ((result.flags & ChnkCellFlags::HeightMap) && r.can(722)) { hasHeightmap = true; heightmap.resize(19 * 19); for (int j = 0; j < 19 * 19; j++) heightmap[j] = r.u16le(); }
    if ((result.flags & ChnkCellFlags::WorldLayerIDs) && r.can(16)) { for (int j = 0; j < 4; j++) result.worldLayerIDs[j] = r.u32le(); }
    if ((result.flags & ChnkCellFlags::BlendMap) && r.can(8450)) { result.blendMap.resize(65 * 65 * 4); for (int i = 0; i < 65 * 65; i++) { uint16 val = r.u16le(); result.blendMap[i * 4 + 0] = static_cast<uint8>(((val >> 0) & 0xF) * 255 / 15); result.blendMap[i * 4 + 1] = static_cast<uint8>(((val >> 4) & 0xF) * 255 / 15); result.blendMap[i * 4 + 2] = static_cast<uint8>(((val >> 8) & 0xF) * 255 / 15); result.blendMap[i * 4 + 3] = static_cast<uint8>(((val >> 12) & 0xF) * 255 / 15); } }
    if ((result.flags & ChnkCellFlags::ColorMap) && r.can(8450)) { result.colorMap.resize(65 * 65 * 4); for (int i = 0; i < 65 * 65; i++) { uint16 val = r.u16le(); uint8 r5 = (val >> 0) & 0x1F; uint8 g6 = (val >> 5) & 0x3F; uint8 b5 = (val >> 11) & 0x1F; result.colorMap[i * 4 + 0] = static_cast<uint8>((r5 * 255) / 31); result.colorMap[i * 4 + 1] = static_cast<uint8>((g6 * 255) / 63); result.colorMap[i * 4 + 2] = static_cast<uint8>((b5 * 255) / 31); result.colorMap[i * 4 + 3] = 255; } }
    if ((result.flags & ChnkCellFlags::UnkMap) && r.can(8450)) { result.unknownMap.resize(65 * 65); for (int j = 0; j < 65 * 65; j++) result.unknownMap[j] = r.u16le(); }
    if ((result.flags & ChnkCellFlags::Unk0x20) && r.can(4)) result.unk0x20 = static_cast<int32>(r.u32le());
    if ((result.flags & ChnkCellFlags::SkyIDs) && r.can(64)) { for (int corner = 0; corner < 4; corner++) for (int skyIdx = 0; skyIdx < 4; skyIdx++) result.skyCorners[corner].worldSkyIDs[skyIdx] = r.u32le(); }
    if ((result.flags & ChnkCellFlags::SkyWeights) && r.can(16)) { for (int corner = 0; corner < 4; corner++) for (int weightIdx = 0; weightIdx < 4; weightIdx++) result.skyCorners[corner].worldSkyWeights[weightIdx] = r.u8(); }
    if ((result.flags & ChnkCellFlags::ShadowMap) && r.can(4225)) { result.shadowMap.resize(65 * 65); for (int j = 0; j < 65 * 65; j++) result.shadowMap[j] = r.u8(); }
    if ((result.flags & ChnkCellFlags::LoDHeightMap) && r.can(2178)) { result.lodHeightMap.resize(33 * 33); for (int j = 0; j < 33 * 33; j++) result.lodHeightMap[j] = r.u16le(); }
    if ((result.flags & ChnkCellFlags::LoDHeightRange) && r.can(4)) { result.lodHeightRange[0] = r.u16le(); result.lodHeightRange[1] = r.u16le(); }
    if ((result.flags & ChnkCellFlags::Unk0x800) && r.can(578)) r.skip(578);
    if ((result.flags & ChnkCellFlags::Unk0x1000) && r.can(1)) r.skip(1);
    if ((result.flags & ChnkCellFlags::ColorMapDXT) && r.can(4624)) { result.colorMapDXT.resize(4624); for (int j = 0; j < 4624; j++) result.colorMapDXT[j] = r.u8(); }
    if ((result.flags & ChnkCellFlags::UnkMap0DXT) && r.can(2312)) r.skip(2312);
    if ((result.flags & ChnkCellFlags::Unk0x8000) && r.can(8450)) r.skip(8450);
    if ((result.flags & ChnkCellFlags::ZoneBound) && r.can(4096)) { result.zoneBounds.resize(64 * 64); for (int j = 0; j < 64 * 64; j++) result.zoneBounds[j] = r.u8(); }
    if ((result.flags & ChnkCellFlags::BlendMapDXT) && r.can(2312)) { result.blendMapDXT.resize(2312); for (int j = 0; j < 2312; j++) result.blendMapDXT[j] = r.u8(); }
    if ((result.flags & ChnkCellFlags::UnkMap1DXT) && r.can(2312)) { result.unkMap1.resize(2312); for (int j = 0; j < 2312; j++) result.unkMap1[j] = r.u8(); }
    if ((result.flags & ChnkCellFlags::UnkMap2DXT) && r.can(2312)) r.skip(2312);
    if ((result.flags & ChnkCellFlags::UnkMap3DXT) && r.can(2312)) r.skip(2312);
    if ((result.flags & ChnkCellFlags::Unk0x200000) && r.can(1)) r.skip(1);
    if ((result.flags & ChnkCellFlags::Unk0x400000) && r.can(16)) r.skip(16);
    if ((result.flags & ChnkCellFlags::Unk0x800000) && r.can(16900)) r.skip(16900);
    if ((result.flags & ChnkCellFlags::Unk0x1000000) && r.can(8)) r.skip(8);
    if ((result.flags & ChnkCellFlags::Unk0x2000000) && r.can(8450)) r.skip(8450);
    if ((result.flags & ChnkCellFlags::Unk0x4000000) && r.can(21316)) r.skip(21316);
    if ((result.flags & ChnkCellFlags::Unk0x8000000) && r.can(4096)) r.skip(4096);
    if ((result.flags & ChnkCellFlags::Zone) && r.can(16)) { for (int j = 0; j < 4; j++) result.zoneIds[j] = r.u32le(); }
    if ((result.flags & ChnkCellFlags::Unk0x20000000) && r.can(8450)) r.skip(8450);
    if ((result.flags & ChnkCellFlags::Unk0x40000000) && r.can(8450)) r.skip(8450);
    if ((result.flags & ChnkCellFlags::UnkMap4DXT) && r.can(2312)) r.skip(2312);
    while (r.can(8))
    {
        uint32 chunkID = r.u32le();
        uint32 chunkSize = r.u32le();
        if (!r.can(chunkSize)) break;
        switch (chunkID)
        {
            case 0x504F5250: { uint32 propCount = chunkSize / 4; result.props.uniqueIDs.resize(propCount); for (uint32 i = 0; i < propCount; i++) result.props.uniqueIDs[i] = r.u32le(); break; }
            case 0x44727563: { result.curd.rawData.resize(chunkSize); for (uint32 i = 0; i < chunkSize; i++) result.curd.rawData[i] = r.u8(); break; }
            case 0x47744157: { if (chunkSize >= 4) { uint32 waterCount = r.u32le(); if (waterCount > 0 && waterCount < 1000) { result.waters.resize(waterCount); uint32 remainingBytes = chunkSize - 4; uint32 bytesPerWater = waterCount > 0 ? remainingBytes / waterCount : 0; for (uint32 i = 0; i < waterCount && r.can(bytesPerWater); i++) { result.waters[i].rawData.resize(bytesPerWater); for (uint32 j = 0; j < bytesPerWater; j++) result.waters[i].rawData[j] = r.u8(); } result.hasWater = true; } } break; }
            default: r.skip(chunkSize); break;
        }
    }
    if (result.blendMap.empty() && result.blendMapDXT.empty()) { result.blendMap.resize(65 * 65 * 4); for (int i = 0; i < 65 * 65; i++) { result.blendMap[i * 4 + 0] = 255; result.blendMap[i * 4 + 1] = 0; result.blendMap[i * 4 + 2] = 0; result.blendMap[i * 4 + 3] = 0; } }
    if (result.colorMap.empty() && result.colorMapDXT.empty()) { result.colorMap.resize(65 * 65 * 4); for (int i = 0; i < 65 * 65; i++) { result.colorMap[i * 4 + 0] = 128; result.colorMap[i * 4 + 1] = 128; result.colorMap[i * 4 + 2] = 128; result.colorMap[i * 4 + 3] = 255; } }
    if (!hasHeightmap) return result;
    float baseX = static_cast<float>(cellX) * 32.0f;
    float baseZ = static_cast<float>(cellY) * 32.0f;
    float totalHeight = 0.0f;
    uint32 validHeights = 0;
    result.vertices.resize(17 * 17);
    for (int y = 0; y < 17; y++)
    {
        for (int x = 0; x < 17; x++)
        {
            uint16 h = heightmap[y * 19 + x] & 0x7FFF;
            float height = (static_cast<float>(h) / 8.0f) - 2048.0f;
            AreaVertex v{};
            v.x = baseX + static_cast<float>(x) * UnitSize;
            v.z = baseZ + static_cast<float>(y) * UnitSize;
            v.y = height;
            v.u = static_cast<float>(x) / 16.0f;
            v.v = static_cast<float>(y) / 16.0f;
            v.nx = 0.0f; v.ny = 1.0f; v.nz = 0.0f;
            v.tanx = 1.0f; v.tany = 0.0f; v.tanz = 0.0f; v.tanw = 1.0f;
            if (height > result.maxHeight) result.maxHeight = height;
            totalHeight += height;
            validHeights++;
            glm::vec3 pos(v.x, v.y, v.z);
            result.minBounds = glm::min(result.minBounds, pos);
            result.maxBounds = glm::max(result.maxBounds, pos);
            result.vertices[y * 17 + x] = v;
        }
    }
    if (validHeights > 0) result.avgHeight = totalHeight / static_cast<float>(validHeights);
    for (int y = 0; y < 17; y++)
    {
        for (int x = 0; x < 17; x++)
        {
            auto getPos = [&](int px, int py) -> glm::vec3 { px = std::clamp(px, 0, 16); py = std::clamp(py, 0, 16); auto& vtx = result.vertices[py * 17 + px]; return glm::vec3(vtx.x, vtx.y, vtx.z); };
            glm::vec3 left = getPos(x - 1, y);
            glm::vec3 right = getPos(x + 1, y);
            glm::vec3 up = getPos(x, y - 1);
            glm::vec3 down = getPos(x, y + 1);
            glm::vec3 dx = right - left;
            glm::vec3 dz = down - up;
            glm::vec3 normal = glm::normalize(glm::cross(dz, dx));
            result.vertices[y * 17 + x].nx = normal.x;
            result.vertices[y * 17 + x].ny = normal.y;
            result.vertices[y * 17 + x].nz = normal.z;
        }
    }
    result.valid = true;
    return result;
}

AreaChunkRender::AreaChunkRender(ParsedChunk&& parsed, ArchivePtr)
    : mFlags(parsed.flags), mMinBounds(parsed.minBounds), mMaxBounds(parsed.maxBounds), mSplatTexture(0), mColorMapTexture(0)
{
    mMaxHeight = parsed.maxHeight;
    mAverageHeight = parsed.avgHeight;
    for (int i = 0; i < 4; i++) { mWorldLayerIDs[i] = parsed.worldLayerIDs[i]; mZoneIds[i] = parsed.zoneIds[i]; mLayerScale[i] = 0.0f; }
    mVertices = std::move(parsed.vertices);
    mBlendMap = std::move(parsed.blendMap);
    mBlendMapDXT = std::move(parsed.blendMapDXT);
    mColorMap = std::move(parsed.colorMap);
    mColorMapDXT = std::move(parsed.colorMapDXT);
    mUnknownMap = std::move(parsed.unknownMap);
    mUnk0x20 = parsed.unk0x20;
    for (int i = 0; i < 4; i++) mSkyCorners[i] = parsed.skyCorners[i];
    mShadowMap = std::move(parsed.shadowMap);
    mLodHeightMap = std::move(parsed.lodHeightMap);
    mLodHeightRange[0] = parsed.lodHeightRange[0];
    mLodHeightRange[1] = parsed.lodHeightRange[1];
    mZoneBounds = std::move(parsed.zoneBounds);
    mUnkMap1 = std::move(parsed.unkMap1);
    mProps = std::move(parsed.props);
    mCurd = std::move(parsed.curd);
    mWaters = std::move(parsed.waters);
    mHasWater = parsed.hasWater;
    if (mVertices.empty()) return;
    uploadGPU();
}

AreaChunkRender::AreaChunkRender(const std::vector<uint8>& cellData, uint32 cellX, uint32 cellY, ArchivePtr)
    : mChunkData(cellData), mFlags(0), mMinBounds(std::numeric_limits<float>::max()), mMaxBounds(std::numeric_limits<float>::lowest()), mSplatTexture(0), mColorMapTexture(0)
{
    for (int i = 0; i < 4; ++i) mLayerScale[i] = 0.0f;
    if (cellData.size() < 4) return;
    struct R
    {
        const uint8* p; const uint8* e;
        bool can(size_t n) const { return static_cast<size_t>(e - p) >= n; }
        uint32 u32le() { uint32 v = static_cast<uint32>(p[0]) | (static_cast<uint32>(p[1]) << 8) | (static_cast<uint32>(p[2]) << 16) | (static_cast<uint32>(p[3]) << 24); p += 4; return v; }
        uint16 u16le() { uint16 v = static_cast<uint16>(p[0]) | (static_cast<uint16>(p[1]) << 8); p += 2; return v; }
        uint8 u8() { return *p++; }
        void skip(size_t n) { p += n; }
        size_t remaining() const { return static_cast<size_t>(e - p); }
    };
    R r{ cellData.data(), cellData.data() + cellData.size() };
    mFlags = r.u32le();
    std::vector<uint16> heightmap;
    bool hasHeightmap = false;
    if ((mFlags & ChnkCellFlags::HeightMap) && r.can(722)) { hasHeightmap = true; heightmap.resize(19 * 19); for (int j = 0; j < 19 * 19; j++) heightmap[j] = r.u16le(); }
    if ((mFlags & ChnkCellFlags::WorldLayerIDs) && r.can(16)) { for (int j = 0; j < 4; j++) mWorldLayerIDs[j] = r.u32le(); }
    if ((mFlags & ChnkCellFlags::BlendMap) && r.can(8450)) { mBlendMap.resize(65 * 65 * 4); for (int i = 0; i < 65 * 65; i++) { uint16 val = r.u16le(); mBlendMap[i * 4 + 0] = static_cast<uint8>(((val >> 0) & 0xF) * 255 / 15); mBlendMap[i * 4 + 1] = static_cast<uint8>(((val >> 4) & 0xF) * 255 / 15); mBlendMap[i * 4 + 2] = static_cast<uint8>(((val >> 8) & 0xF) * 255 / 15); mBlendMap[i * 4 + 3] = static_cast<uint8>(((val >> 12) & 0xF) * 255 / 15); } }
    if ((mFlags & ChnkCellFlags::ColorMap) && r.can(8450)) { mColorMap.resize(65 * 65 * 4); for (int i = 0; i < 65 * 65; i++) { uint16 val = r.u16le(); uint8 r5 = (val >> 0) & 0x1F; uint8 g6 = (val >> 5) & 0x3F; uint8 b5 = (val >> 11) & 0x1F; mColorMap[i * 4 + 0] = static_cast<uint8>((r5 * 255) / 31); mColorMap[i * 4 + 1] = static_cast<uint8>((g6 * 255) / 63); mColorMap[i * 4 + 2] = static_cast<uint8>((b5 * 255) / 31); mColorMap[i * 4 + 3] = 255; } }
    if ((mFlags & ChnkCellFlags::UnkMap) && r.can(8450)) { mUnknownMap.resize(65 * 65); for (int j = 0; j < 65 * 65; j++) mUnknownMap[j] = r.u16le(); }
    if ((mFlags & ChnkCellFlags::Unk0x20) && r.can(4)) mUnk0x20 = static_cast<int32>(r.u32le());
    if ((mFlags & ChnkCellFlags::SkyIDs) && r.can(64)) { for (int corner = 0; corner < 4; corner++) for (int skyIdx = 0; skyIdx < 4; skyIdx++) mSkyCorners[corner].worldSkyIDs[skyIdx] = r.u32le(); }
    if ((mFlags & ChnkCellFlags::SkyWeights) && r.can(16)) { for (int corner = 0; corner < 4; corner++) for (int weightIdx = 0; weightIdx < 4; weightIdx++) mSkyCorners[corner].worldSkyWeights[weightIdx] = r.u8(); }
    if ((mFlags & ChnkCellFlags::ShadowMap) && r.can(4225)) { mShadowMap.resize(65 * 65); for (int j = 0; j < 65 * 65; j++) mShadowMap[j] = r.u8(); }
    if ((mFlags & ChnkCellFlags::LoDHeightMap) && r.can(2178)) { mLodHeightMap.resize(33 * 33); for (int j = 0; j < 33 * 33; j++) mLodHeightMap[j] = r.u16le(); }
    if ((mFlags & ChnkCellFlags::LoDHeightRange) && r.can(4)) { mLodHeightRange[0] = r.u16le(); mLodHeightRange[1] = r.u16le(); }
    if ((mFlags & ChnkCellFlags::Unk0x800) && r.can(578)) r.skip(578);
    if ((mFlags & ChnkCellFlags::Unk0x1000) && r.can(1)) r.skip(1);
    if ((mFlags & ChnkCellFlags::ColorMapDXT) && r.can(4624)) { mColorMapDXT.resize(4624); for (int j = 0; j < 4624; j++) mColorMapDXT[j] = r.u8(); }
    if ((mFlags & ChnkCellFlags::UnkMap0DXT) && r.can(2312)) r.skip(2312);
    if ((mFlags & ChnkCellFlags::Unk0x8000) && r.can(8450)) r.skip(8450);
    if ((mFlags & ChnkCellFlags::ZoneBound) && r.can(4096)) { mZoneBounds.resize(64 * 64); for (int j = 0; j < 64 * 64; j++) mZoneBounds[j] = r.u8(); }
    if ((mFlags & ChnkCellFlags::BlendMapDXT) && r.can(2312)) { mBlendMapDXT.resize(2312); for (int j = 0; j < 2312; j++) mBlendMapDXT[j] = r.u8(); }
    if ((mFlags & ChnkCellFlags::UnkMap1DXT) && r.can(2312)) { mUnkMap1.resize(2312); for (int j = 0; j < 2312; j++) mUnkMap1[j] = r.u8(); }
    if ((mFlags & ChnkCellFlags::UnkMap2DXT) && r.can(2312)) r.skip(2312);
    if ((mFlags & ChnkCellFlags::UnkMap3DXT) && r.can(2312)) r.skip(2312);
    if ((mFlags & ChnkCellFlags::Unk0x200000) && r.can(1)) r.skip(1);
    if ((mFlags & ChnkCellFlags::Unk0x400000) && r.can(16)) r.skip(16);
    if ((mFlags & ChnkCellFlags::Unk0x800000) && r.can(16900)) r.skip(16900);
    if ((mFlags & ChnkCellFlags::Unk0x1000000) && r.can(8)) r.skip(8);
    if ((mFlags & ChnkCellFlags::Unk0x2000000) && r.can(8450)) r.skip(8450);
    if ((mFlags & ChnkCellFlags::Unk0x4000000) && r.can(21316)) r.skip(21316);
    if ((mFlags & ChnkCellFlags::Unk0x8000000) && r.can(4096)) r.skip(4096);
    if ((mFlags & ChnkCellFlags::Zone) && r.can(16)) { for (int j = 0; j < 4; j++) mZoneIds[j] = r.u32le(); }
    if ((mFlags & ChnkCellFlags::Unk0x20000000) && r.can(8450)) r.skip(8450);
    if ((mFlags & ChnkCellFlags::Unk0x40000000) && r.can(8450)) r.skip(8450);
    if ((mFlags & ChnkCellFlags::UnkMap4DXT) && r.can(2312)) r.skip(2312);
    while (r.can(8))
    {
        uint32 chunkID = r.u32le();
        uint32 chunkSize = r.u32le();
        if (!r.can(chunkSize)) break;
        switch (chunkID)
        {
            case 0x504F5250: { uint32 propCount = chunkSize / 4; mProps.uniqueIDs.resize(propCount); for (uint32 i = 0; i < propCount; i++) mProps.uniqueIDs[i] = r.u32le(); break; }
            case 0x44727563: { mCurd.rawData.resize(chunkSize); for (uint32 i = 0; i < chunkSize; i++) mCurd.rawData[i] = r.u8(); break; }
            case 0x47744157: { if (chunkSize >= 4) { uint32 waterCount = r.u32le(); if (waterCount > 0 && waterCount < 1000) { mWaters.resize(waterCount); uint32 remainingBytes = chunkSize - 4; uint32 bytesPerWater = waterCount > 0 ? remainingBytes / waterCount : 0; for (uint32 i = 0; i < waterCount && r.can(bytesPerWater); i++) { mWaters[i].rawData.resize(bytesPerWater); for (uint32 j = 0; j < bytesPerWater; j++) mWaters[i].rawData[j] = r.u8(); } mHasWater = true; } } break; }
            default: r.skip(chunkSize); break;
        }
    }
    if (mBlendMap.empty() && mBlendMapDXT.empty()) { mBlendMap.resize(65 * 65 * 4); for (int i = 0; i < 65 * 65; i++) { mBlendMap[i * 4 + 0] = 255; mBlendMap[i * 4 + 1] = 0; mBlendMap[i * 4 + 2] = 0; mBlendMap[i * 4 + 3] = 0; } }
    if (mColorMap.empty() && mColorMapDXT.empty()) { mColorMap.resize(65 * 65 * 4); for (int i = 0; i < 65 * 65; i++) { mColorMap[i * 4 + 0] = 128; mColorMap[i * 4 + 1] = 128; mColorMap[i * 4 + 2] = 128; mColorMap[i * 4 + 3] = 255; } }
    if (!hasHeightmap) return;
    float baseX = static_cast<float>(cellX) * 32.0f;
    float baseZ = static_cast<float>(cellY) * 32.0f;
    float totalHeight = 0.0f;
    uint32 validHeights = 0;
    mVertices.resize(17 * 17);
    for (int y = 0; y < 17; y++)
    {
        for (int x = 0; x < 17; x++)
        {
            uint16 h = heightmap[y * 19 + x] & 0x7FFF;
            float height = (static_cast<float>(h) / 8.0f) - 2048.0f;
            AreaVertex v{};
            v.x = baseX + static_cast<float>(x) * UnitSize;
            v.z = baseZ + static_cast<float>(y) * UnitSize;
            v.y = height;
            v.u = static_cast<float>(x) / 16.0f;
            v.v = static_cast<float>(y) / 16.0f;
            v.nx = 0.0f; v.ny = 1.0f; v.nz = 0.0f;
            v.tanx = 1.0f; v.tany = 0.0f; v.tanz = 0.0f; v.tanw = 1.0f;
            if (height > mMaxHeight) mMaxHeight = height;
            totalHeight += height;
            validHeights++;
            glm::vec3 pos(v.x, v.y, v.z);
            mMinBounds = glm::min(mMinBounds, pos);
            mMaxBounds = glm::max(mMaxBounds, pos);
            mVertices[y * 17 + x] = v;
        }
    }
    if (validHeights > 0) mAverageHeight = totalHeight / static_cast<float>(validHeights);
    calcNormals();
    uploadGPU();
}

void AreaChunkRender::uploadGPU()
{
    if (indices.empty())
    {
        indices.reserve(16 * 16 * 6);
        for (uint32 y = 0; y < 16; y++)
        {
            for (uint32 x = 0; x < 16; x++)
            {
                uint32 tl = y * 17 + x;
                uint32 tr = y * 17 + x + 1;
                uint32 bl = (y + 1) * 17 + x;
                uint32 br = (y + 1) * 17 + x + 1;
                indices.push_back(tl);
                indices.push_back(bl);
                indices.push_back(tr);
                indices.push_back(tr);
                indices.push_back(bl);
                indices.push_back(br);
            }
        }
    }
    glGenVertexArrays(1, &mVAO);
    glGenBuffers(1, &mVBO);
    glGenBuffers(1, &mEBO);
    glBindVertexArray(mVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(AreaVertex), mVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32), indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AreaVertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(AreaVertex), (void*)offsetof(AreaVertex, nx));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(AreaVertex), (void*)offsetof(AreaVertex, tanx));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(AreaVertex), (void*)offsetof(AreaVertex, u));
    glBindVertexArray(0);
    mIndexCount = static_cast<int>(indices.size());
}

AreaChunkRender::~AreaChunkRender()
{
    if (mSplatTexture != 0) glDeleteTextures(1, &mSplatTexture);
    if (mColorMapTexture != 0) glDeleteTextures(1, &mColorMapTexture);
    for (auto tex : mLayerTextures) if (tex != 0) glDeleteTextures(1, &tex);
    if (mBlendMapTexture) glDeleteTextures(1, &mBlendMapTexture);
    if (mColorMapTextureGPU) glDeleteTextures(1, &mColorMapTextureGPU);
    if (mVAO) glDeleteVertexArrays(1, &mVAO);
    if (mVBO) glDeleteBuffers(1, &mVBO);
    if (mEBO) glDeleteBuffers(1, &mEBO);
}

void AreaChunkRender::render()
{
    if (!mVAO) return;
    glBindVertexArray(mVAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void AreaChunkRender::calcNormals()
{
    for (int y = 0; y < 17; y++)
    {
        for (int x = 0; x < 17; x++)
        {
            glm::vec3 normal(0.0f, 0.0f, 0.0f);
            auto getPos = [&](int px, int py) -> glm::vec3 { px = std::clamp(px, 0, 16); py = std::clamp(py, 0, 16); auto& v = mVertices[py * 17 + px]; return glm::vec3(v.x, v.y, v.z); };
            glm::vec3 left = getPos(x - 1, y);
            glm::vec3 right = getPos(x + 1, y);
            glm::vec3 up = getPos(x, y - 1);
            glm::vec3 down = getPos(x, y + 1);
            glm::vec3 dx = right - left;
            glm::vec3 dz = down - up;
            normal = glm::normalize(glm::cross(dz, dx));
            mVertices[y * 17 + x].nx = normal.x;
            mVertices[y * 17 + x].ny = normal.y;
            mVertices[y * 17 + x].nz = normal.z;
        }
    }
}

void AreaChunkRender::calcTangentBitangent() {}
void AreaChunkRender::extendBuffer() {}

void AreaChunkRender::geometryInit(uint32 program)
{
    uniforms.colorTexture = glGetUniformLocation(program, "colorTexture");
    uniforms.alphaTexture = glGetUniformLocation(program, "alphaTexture");
    uniforms.hasColorMap = glGetUniformLocation(program, "hasColorMap");
    uniforms.texScale = glGetUniformLocation(program, "texScale");
    uniforms.camPosition = glGetUniformLocation(program, "camPosition");
    uniforms.model = glGetUniformLocation(program, "model");
    uniforms.highlightColor = glGetUniformLocation(program, "highlightColor");
    uniforms.baseColor = glGetUniformLocation(program, "baseColor");
    for (int i = 0; i < 4; ++i)
    {
        std::string t = "textures[" + std::to_string(i) + "]";
        std::string n = "normalTextures[" + std::to_string(i) + "]";
        uniforms.textures[i] = glGetUniformLocation(program, t.c_str());
        uniforms.normalTextures[i] = glGetUniformLocation(program, n.c_str());
    }
}