#include "AreaFile.h"
#include "TerrainTexture.h"
#include "TerrainShader.h"
#include "Props.h"
#include "../models/M3Loader.h"
#include "../models/M3Render.h"
#include <cmath>
#include <algorithm>
#include <string>
#include <memory>
#include <cstring>
#include <limits>
#include <thread>
#include <chrono>

using namespace DirectX;

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
ComPtr<ID3D11Buffer> AreaChunkRender::sIndexBuffer;
ID3D11Device* AreaChunkRender::sDevice = nullptr;
ID3D11DeviceContext* AreaChunkRender::sContext = nullptr;

void AreaChunkRender::SetDevice(ID3D11Device* device, ID3D11DeviceContext* context)
{
    sDevice = device;
    sContext = context;
}

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
    mWorldOffset.x = static_cast<float>(mTileY - gReferenceTileY) * GRID_SIZE;
    mWorldOffset.y = 0.0f;
    mWorldOffset.z = static_cast<float>(mTileX - gReferenceTileX) * GRID_SIZE;
}

AreaFile::AreaFile(ArchivePtr archive, FileEntryPtr file)
    : mArchive(std::move(archive)), mFile(std::move(file))
    , mMinBounds(FLT_MAX, FLT_MAX, FLT_MAX)
    , mMaxBounds(-FLT_MAX, -FLT_MAX, -FLT_MAX)
    , mBaseColor(1.0f, 1.0f, 1.0f, 1.0f)
    , mWorldOffset(0.0f, 0.0f, 0.0f)
{
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

AreaFile::~AreaFile() = default;

bool AreaFile::loadTexture() { return false; }

XMFLOAT3 AreaFile::getWorldMinBounds() const
{
    return XMFLOAT3(mMinBounds.x + mWorldOffset.x, mMinBounds.y + mWorldOffset.y, mMinBounds.z + mWorldOffset.z);
}

XMFLOAT3 AreaFile::getWorldMaxBounds() const
{
    return XMFLOAT3(mMaxBounds.x + mWorldOffset.x, mMaxBounds.y + mWorldOffset.y, mMaxBounds.z + mWorldOffset.z);
}

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
    if (chnkData.empty()) { mMinBounds = XMFLOAT3(0, 0, 0); mMaxBounds = XMFLOAT3(512, 50, 512); return true; }

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
            XMFLOAT3 cmin = chunk->getMinBounds();
            XMFLOAT3 cmax = chunk->getMaxBounds();
            mMinBounds.x = std::min(mMinBounds.x, cmin.x);
            mMinBounds.y = std::min(mMinBounds.y, cmin.y);
            mMinBounds.z = std::min(mMinBounds.z, cmin.z);
            mMaxBounds.x = std::max(mMaxBounds.x, cmax.x);
            mMaxBounds.y = std::max(mMaxBounds.y, cmax.y);
            mMaxBounds.z = std::max(mMaxBounds.z, cmax.z);
        }
    }

    if (validCount > 0) mAverageHeight = totalH / static_cast<float>(validCount);
    else { mMinBounds = XMFLOAT3(0, 0, 0); mMaxBounds = XMFLOAT3(512, 50, 512); }
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
            XMFLOAT3 cmin = chunk->getMinBounds();
            XMFLOAT3 cmax = chunk->getMaxBounds();
            mMinBounds.x = std::min(mMinBounds.x, cmin.x);
            mMinBounds.y = std::min(mMinBounds.y, cmin.y);
            mMinBounds.z = std::min(mMinBounds.z, cmin.z);
            mMaxBounds.x = std::max(mMaxBounds.x, cmax.x);
            mMaxBounds.y = std::max(mMaxBounds.y, cmax.y);
            mMaxBounds.z = std::max(mMaxBounds.z, cmax.z);
        }
    }
    if (validCount > 0) mAverageHeight = totalH / static_cast<float>(validCount);
    else { mMinBounds = XMFLOAT3(0, 0, 0); mMaxBounds = XMFLOAT3(512, 50, 512); }
    return true;
}

void AreaFile::render(ID3D11DeviceContext* context, const Matrix& matView, const Matrix& matProj, ID3D11Buffer* constantBuffer, const AreaChunkRenderPtr& selectedChunk)
{
    if (!context || !constantBuffer) return;

    XMVECTOR offsetVec = XMLoadFloat3(&mWorldOffset);
    XMMATRIX worldModel = XMMatrixTranslationFromVector(offsetVec);

    XMFLOAT3 center(256.0f, 0.0f, 256.0f);
    XMVECTOR centerVec = XMLoadFloat3(&center);

    worldModel = XMMatrixMultiply(worldModel, XMMatrixTranslationFromVector(centerVec));

    if (mGlobalRotation != 0.0f)
        worldModel = XMMatrixMultiply(worldModel, XMMatrixRotationY(XMConvertToRadians(mGlobalRotation)));

    if (mMirrorX || mMirrorZ)
    {
        float scaleX = mMirrorX ? -1.0f : 1.0f;
        float scaleZ = mMirrorZ ? -1.0f : 1.0f;
        worldModel = XMMatrixMultiply(worldModel, XMMatrixScaling(scaleX, 1.0f, scaleZ));
    }

    worldModel = XMMatrixMultiply(worldModel, XMMatrixTranslationFromVector(XMVectorNegate(centerVec)));

    for (auto& c : mChunks)
    {
        if (!c || !c->isFullyInitialized()) continue;
        c->loadTextures(mArchive);

        TerrainShader::TerrainCB cb;
        cb.view = matView;
        cb.projection = matProj;
        cb.model = worldModel;

        const float* scales = c->getLayerScales();
        float s0 = scales[0] > 0.0f ? 32.0f / scales[0] : 8.0f;
        float s1 = scales[1] > 0.0f ? 32.0f / scales[1] : 8.0f;
        float s2 = scales[2] > 0.0f ? 32.0f / scales[2] : 8.0f;
        float s3 = scales[3] > 0.0f ? 32.0f / scales[3] : 8.0f;
        cb.texScale = XMFLOAT4(s0, s1, s2, s3);

        cb.baseColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

        if (c == selectedChunk)
            cb.highlightColor = XMFLOAT4(1.0f, 1.0f, 0.0f, 0.5f);
        else
            cb.highlightColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);

        cb.camPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
        cb.hasColorMap = c->hasColorMap() ? 1 : 0;

        TerrainShader::UpdateConstants(context, constantBuffer, cb);

        c->bindTextures(context);
        c->render(context);
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

void AreaFile::loadPropsInView(const XMFLOAT3& cameraPos, float radius)
{
    auto& loader = PropLoader::Instance();
    loader.SetArchive(mArchive);
    std::vector<std::pair<float, Prop*>> sorted;
    sorted.reserve(mProps.size());
    float radiusSq = radius * radius;
    for (auto& prop : mProps)
    {
        if (prop.loaded || prop.loadRequested) continue;
        XMFLOAT3 propWorldPos;
        propWorldPos.x = prop.position.x + mWorldOffset.x;
        propWorldPos.y = prop.position.y + mWorldOffset.y;
        propWorldPos.z = prop.position.z + mWorldOffset.z;
        float dx = propWorldPos.x - cameraPos.x;
        float dy = propWorldPos.y - cameraPos.y;
        float dz = propWorldPos.z - cameraPos.z;
        float distSq = dx*dx + dy*dy + dz*dz;
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
    for (const auto& prop : mProps)
    {
        if (!prop.loaded || !prop.render) continue;

        float x = prop.position.x + mWorldOffset.x;
        float y = prop.position.y + mWorldOffset.y;
        float z = prop.position.z + mWorldOffset.z;

        XMMATRIX translation = XMMatrixTranslation(x, y, z);
        XMMATRIX scale = XMMatrixScaling(prop.scale, prop.scale, prop.scale);

        XMVECTOR quat = XMVectorSet(prop.rotation.x, prop.rotation.y, prop.rotation.z, prop.rotation.w);
        XMMATRIX rotation = XMMatrixRotationQuaternion(quat);

        XMMATRIX model = scale * rotation * translation;

        prop.render->render(matView, matProj, model);
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
        result.minBounds = XMFLOAT3(0, 0, 0);
        result.maxBounds = XMFLOAT3(512, 50, 512);
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
            result.minBounds.x = std::min(result.minBounds.x, chunk.minBounds.x);
            result.minBounds.y = std::min(result.minBounds.y, chunk.minBounds.y);
            result.minBounds.z = std::min(result.minBounds.z, chunk.minBounds.z);
            result.maxBounds.x = std::max(result.maxBounds.x, chunk.maxBounds.x);
            result.maxBounds.y = std::max(result.maxBounds.y, chunk.maxBounds.y);
            result.maxBounds.z = std::max(result.maxBounds.z, chunk.maxBounds.z);
        }
        result.chunks[index] = std::move(chunk);
    }
    if (validCount > 0) result.avgHeight = totalH / static_cast<float>(validCount);
    else { result.minBounds = XMFLOAT3(0, 0, 0); result.maxBounds = XMFLOAT3(512, 50, 512); }
    result.valid = true;
    return result;
}

void AreaChunkRender::loadTextures(const ArchivePtr& archive)
{
    if (mTexturesLoaded) return;
    auto& texMgr = TerrainTexture::Manager::Instance();
    texMgr.LoadWorldLayerTable(archive);

    if (!mBlendMap.empty())
        mBlendMapSRV = texMgr.CreateBlendMapTexture(mBlendMap.data(), 65, 65);
    else if (!mBlendMapDXT.empty())
        mBlendMapSRV = texMgr.CreateBlendMapFromDXT1(mBlendMapDXT.data(), mBlendMapDXT.size(), 65, 65);

    if (!mColorMap.empty())
        mColorMapSRV = texMgr.CreateColorMapTexture(mColorMap.data(), 65, 65);
    else if (!mColorMapDXT.empty())
        mColorMapSRV = texMgr.CreateColorMapFromDXT5(mColorMapDXT.data(), mColorMapDXT.size(), 65, 65);

    for (int i = 0; i < 4; ++i)
    {
        uint32_t layerId = mWorldLayerIDs[i];
        if (layerId == 0)
        {
            mLayerDiffuse[i] = nullptr;
            mLayerNormal[i] = nullptr;
            mLayerScale[i] = 4.0f;
            continue;
        }
        const auto* cached = texMgr.GetLayerTexture(archive, layerId);
        if (cached && cached->loaded)
        {
            mLayerDiffuse[i] = cached->diffuse;
            mLayerNormal[i] = cached->normal ? cached->normal : nullptr;
        }
        const auto* layerEntry = texMgr.GetLayerEntry(layerId);
        if (layerEntry && layerEntry->scaleU > 0.0f)
            mLayerScale[i] = layerEntry->scaleU;
        else
            mLayerScale[i] = 4.0f;
    }
    mTexturesLoaded = true;
}

void AreaChunkRender::bindTextures(ID3D11DeviceContext* context) const
{
    if (!context) return;

    auto& texMgr = TerrainTexture::Manager::Instance();
    ID3D11ShaderResourceView* fallbackWhite = texMgr.GetFallbackWhite();
    ID3D11ShaderResourceView* fallbackNormal = texMgr.GetFallbackNormal();

    ID3D11ShaderResourceView* srvs[10] = {};

    srvs[0] = mBlendMapSRV ? mBlendMapSRV.Get() : fallbackWhite;
    srvs[1] = mColorMapSRV ? mColorMapSRV.Get() : fallbackWhite;

    for (int i = 0; i < 4; ++i)
        srvs[2 + i] = mLayerDiffuse[i] ? mLayerDiffuse[i].Get() : fallbackWhite;

    for (int i = 0; i < 4; ++i)
        srvs[6 + i] = mLayerNormal[i] ? mLayerNormal[i].Get() : fallbackNormal;

    context->PSSetShaderResources(0, 10, srvs);
}

void AreaChunkRender::render(ID3D11DeviceContext* context)
{
    if (!mVertexBuffer || !sIndexBuffer || !context) return;

    UINT stride = sizeof(AreaVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(sIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->DrawIndexed(static_cast<UINT>(indices.size()), 0, 0);
}

void AreaChunkRender::geometryInit()
{
    if (!sDevice) return;

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

    if (!sIndexBuffer && !indices.empty())
    {
        D3D11_BUFFER_DESC ibd = {};
        ibd.Usage = D3D11_USAGE_IMMUTABLE;
        ibd.ByteWidth = static_cast<UINT>(indices.size() * sizeof(uint32));
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = indices.data();

        sDevice->CreateBuffer(&ibd, &initData, &sIndexBuffer);
    }
}

void AreaChunkRender::calcNormals()
{
    for (int y = 0; y < 17; y++)
    {
        for (int x = 0; x < 17; x++)
        {
            auto getPos = [&](int px, int py) -> XMVECTOR
            {
                px = std::clamp(px, 0, 16);
                py = std::clamp(py, 0, 16);
                auto& v = mVertices[py * 17 + px];
                return XMVectorSet(v.x, v.y, v.z, 0.0f);
            };
            XMVECTOR left = getPos(x - 1, y);
            XMVECTOR right = getPos(x + 1, y);
            XMVECTOR up = getPos(x, y - 1);
            XMVECTOR down = getPos(x, y + 1);
            XMVECTOR dx = XMVectorSubtract(right, left);
            XMVECTOR dz = XMVectorSubtract(down, up);
            XMVECTOR normal = XMVector3Normalize(XMVector3Cross(dz, dx));
            XMFLOAT3 n;
            XMStoreFloat3(&n, normal);
            mVertices[y * 17 + x].nx = n.x;
            mVertices[y * 17 + x].ny = n.y;
            mVertices[y * 17 + x].nz = n.z;
        }
    }
}

void AreaChunkRender::calcTangentBitangent() {}
void AreaChunkRender::extendBuffer() {}

void AreaChunkRender::uploadGPU()
{
    if (!sDevice || mVertices.empty()) return;

    geometryInit();

    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = static_cast<UINT>(mVertices.size() * sizeof(AreaVertex));
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = mVertices.data();

    sDevice->CreateBuffer(&vbd, &initData, &mVertexBuffer);
    mIndexCount = static_cast<int>(indices.size());
}

AreaChunkRender::~AreaChunkRender() = default;

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
            result.minBounds.x = std::min(result.minBounds.x, v.x);
            result.minBounds.y = std::min(result.minBounds.y, v.y);
            result.minBounds.z = std::min(result.minBounds.z, v.z);
            result.maxBounds.x = std::max(result.maxBounds.x, v.x);
            result.maxBounds.y = std::max(result.maxBounds.y, v.y);
            result.maxBounds.z = std::max(result.maxBounds.z, v.z);
            result.vertices[y * 17 + x] = v;
        }
    }
    if (validHeights > 0) result.avgHeight = totalHeight / static_cast<float>(validHeights);
    for (int y = 0; y < 17; y++)
    {
        for (int x = 0; x < 17; x++)
        {
            auto getPos = [&](int px, int py) -> XMFLOAT3 { px = std::clamp(px, 0, 16); py = std::clamp(py, 0, 16); auto& vtx = result.vertices[py * 17 + px]; return XMFLOAT3(vtx.x, vtx.y, vtx.z); };
            XMFLOAT3 left = getPos(x - 1, y);
            XMFLOAT3 right = getPos(x + 1, y);
            XMFLOAT3 up = getPos(x, y - 1);
            XMFLOAT3 down = getPos(x, y + 1);
            XMVECTOR leftV = XMLoadFloat3(&left);
            XMVECTOR rightV = XMLoadFloat3(&right);
            XMVECTOR upV = XMLoadFloat3(&up);
            XMVECTOR downV = XMLoadFloat3(&down);
            XMVECTOR dx = XMVectorSubtract(rightV, leftV);
            XMVECTOR dz = XMVectorSubtract(downV, upV);
            XMVECTOR normal = XMVector3Normalize(XMVector3Cross(dz, dx));
            XMFLOAT3 n;
            XMStoreFloat3(&n, normal);
            result.vertices[y * 17 + x].nx = n.x;
            result.vertices[y * 17 + x].ny = n.y;
            result.vertices[y * 17 + x].nz = n.z;
        }
    }
    result.valid = true;
    return result;
}

AreaChunkRender::AreaChunkRender(ParsedChunk&& parsed, ArchivePtr)
    : mFlags(parsed.flags), mMinBounds(parsed.minBounds), mMaxBounds(parsed.maxBounds)
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
    : mChunkData(cellData), mFlags(0), mMinBounds(FLT_MAX, FLT_MAX, FLT_MAX), mMaxBounds(-FLT_MAX, -FLT_MAX, -FLT_MAX)
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
            mMinBounds.x = std::min(mMinBounds.x, v.x);
            mMinBounds.y = std::min(mMinBounds.y, v.y);
            mMinBounds.z = std::min(mMinBounds.z, v.z);
            mMaxBounds.x = std::max(mMaxBounds.x, v.x);
            mMaxBounds.y = std::max(mMaxBounds.y, v.y);
            mMaxBounds.z = std::max(mMaxBounds.z, v.z);
            mVertices[y * 17 + x] = v;
        }
    }
    if (validHeights > 0) mAverageHeight = totalHeight / static_cast<float>(validHeights);
    calcNormals();
    uploadGPU();
}

void AreaFile::printTransformDebug() const {}
void AreaFile::printAllTransformsDebug(const std::vector<std::shared_ptr<AreaFile>>&) {}
void AreaFile::printRotationDebug() const {}