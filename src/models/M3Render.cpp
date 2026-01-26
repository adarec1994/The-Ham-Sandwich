#define NOMINMAX
#include "M3Render.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <codecvt>
#include <locale>
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <set>
#include <chrono>
#include <iostream>
#include <limits>


extern void RecordTextureFailure(const std::string& modelPath, const std::string& texturePath);

ID3D11Device* M3Render::sDevice = nullptr;
ID3D11DeviceContext* M3Render::sContext = nullptr;
ComPtr<ID3D11VertexShader> M3Render::sSharedVS;
ComPtr<ID3D11PixelShader> M3Render::sSharedPS;
ComPtr<ID3D11InputLayout> M3Render::sSharedLayout;
ComPtr<ID3D11VertexShader> M3Render::sSkeletonVS;
ComPtr<ID3D11PixelShader> M3Render::sSkeletonPS;
ComPtr<ID3D11InputLayout> M3Render::sSkeletonLayout;
ComPtr<ID3D11SamplerState> M3Render::sSharedSampler;
ComPtr<ID3D11RasterizerState> M3Render::sRasterState;
ComPtr<ID3D11DepthStencilState> M3Render::sDepthState;
ComPtr<ID3D11BlendState> M3Render::sBlendState;
ComPtr<ID3D11BlendState> M3Render::sAlphaBlendState;
bool M3Render::sShadersInitialized = false;
std::unordered_map<std::string, ComPtr<ID3D11ShaderResourceView>> M3Render::sTextureSRVCache;
std::mutex M3Render::sTextureCacheMutex;

const char* m3VertexHLSL = R"(
cbuffer M3CB : register(b0) {
    row_major matrix model;
    row_major matrix view;
    row_major matrix projection;
    float3 highlightColor;
    float highlightMix;
    int useSkinning;
    float3 pad;
};
cbuffer BoneBuffer : register(b1) {
    row_major matrix boneMatrices[200];
};
struct VSInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
    float4 boneWeights : BLENDWEIGHT;
    uint4 boneIndices : BLENDINDICES;
};
struct PSInput {
    float4 position : SV_POSITION;
    float3 normal : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
};
PSInput main(VSInput input) {
    PSInput output;
    float4 pos = float4(input.position, 1.0);
    float3 norm = input.normal;
    if (useSkinning == 1) {
        matrix skinMatrix = boneMatrices[input.boneIndices.x] * input.boneWeights.x
                         + boneMatrices[input.boneIndices.y] * input.boneWeights.y
                         + boneMatrices[input.boneIndices.z] * input.boneWeights.z
                         + boneMatrices[input.boneIndices.w] * input.boneWeights.w;
        pos = mul(skinMatrix, pos);
        norm = mul((float3x3)skinMatrix, norm);
    }
    float4 worldPos = mul(model, pos);
    output.position = mul(projection, mul(view, worldPos));
    output.normal = norm;
    output.texCoord = input.texCoord;
    return output;
}
)";

const char* m3PixelHLSL = R"(
cbuffer M3CB : register(b0) {
    row_major matrix model;
    row_major matrix view;
    row_major matrix projection;
    float3 highlightColor;
    float highlightMix;
    int useSkinning;
    float3 pad;
};
Texture2D diffTexture : register(t0);
SamplerState samplerState : register(s0);
struct PSInput {
    float4 position : SV_POSITION;
    float3 normal : TEXCOORD0;
    float2 texCoord : TEXCOORD1;
};
float4 main(PSInput input) : SV_TARGET {
    float3 lightDir = normalize(float3(0.5, 1.0, 0.3));
    float diff = max(dot(normalize(input.normal), lightDir), 0.3);
    float4 texColor = diffTexture.Sample(samplerState, input.texCoord);
    if (texColor.a < 0.5) discard;
    float3 baseColor = texColor.rgb * diff;
    float3 finalColor = lerp(baseColor, highlightColor, highlightMix);
    return float4(finalColor, 1.0);
}
)";

const char* skeletonVertHLSL = R"(
cbuffer SkeletonCB : register(b0) {
    row_major matrix view;
    row_major matrix projection;
};
struct VSInput {
    float3 position : POSITION;
    float3 color : COLOR;
};
struct PSInput {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};
PSInput main(VSInput input) {
    PSInput output;
    output.position = mul(projection, mul(view, float4(input.position, 1.0)));
    output.color = input.color;
    return output;
}
)";

const char* skeletonPixelHLSL = R"(
struct PSInput {
    float4 position : SV_POSITION;
    float3 color : COLOR;
};
float4 main(PSInput input) : SV_TARGET {
    return float4(input.color, 1.0);
}
)";

struct RenderVertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 boneWeights;
    glm::uvec4 boneIndices;
};

static XMMATRIX GlmToXM(const glm::mat4& m) {
    return XMMATRIX(
        m[0][0], m[1][0], m[2][0], m[3][0],
        m[0][1], m[1][1], m[2][1], m[3][1],
        m[0][2], m[1][2], m[2][2], m[3][2],
        m[0][3], m[1][3], m[2][3], m[3][3]
    );
}

void M3Render::SetDevice(ID3D11Device* device, ID3D11DeviceContext* context) {
    sDevice = device;
    sContext = context;
}

void M3Render::InitSharedResources() {
    if (sShadersInitialized || !sDevice) return;

    ComPtr<ID3DBlob> vsBlob, psBlob, errBlob;
    D3DCompile(m3VertexHLSL, strlen(m3VertexHLSL), "M3VS", nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &errBlob);
    D3DCompile(m3PixelHLSL, strlen(m3PixelHLSL), "M3PS", nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errBlob);

    if (vsBlob) sDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &sSharedVS);
    if (psBlob) sDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &sSharedPS);

    if (vsBlob) {
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        sDevice->CreateInputLayout(layout, _countof(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &sSharedLayout);
    }

    D3DCompile(skeletonVertHLSL, strlen(skeletonVertHLSL), "SkeletonVS", nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &errBlob);
    D3DCompile(skeletonPixelHLSL, strlen(skeletonPixelHLSL), "SkeletonPS", nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &errBlob);

    if (vsBlob) sDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &sSkeletonVS);
    if (psBlob) sDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &sSkeletonPS);

    if (vsBlob) {
        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        sDevice->CreateInputLayout(layout, _countof(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &sSkeletonLayout);
    }

    D3D11_SAMPLER_DESC sd = {};
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.MaxAnisotropy = 1;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    sDevice->CreateSamplerState(&sd, &sSharedSampler);

    D3D11_RASTERIZER_DESC rd = {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.FrontCounterClockwise = FALSE;
    rd.DepthClipEnable = TRUE;
    sDevice->CreateRasterizerState(&rd, &sRasterState);

    D3D11_DEPTH_STENCIL_DESC dd = {};
    dd.DepthEnable = TRUE;
    dd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dd.DepthFunc = D3D11_COMPARISON_LESS;
    sDevice->CreateDepthStencilState(&dd, &sDepthState);

    D3D11_BLEND_DESC bd = {};
    bd.RenderTarget[0].BlendEnable = FALSE;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    sDevice->CreateBlendState(&bd, &sBlendState);

    D3D11_BLEND_DESC abd = {};
    abd.RenderTarget[0].BlendEnable = TRUE;
    abd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    abd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    abd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    abd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    abd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    abd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    abd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    sDevice->CreateBlendState(&abd, &sAlphaBlendState);

    sShadersInitialized = true;
}

M3Render::M3Render(const M3ModelData& data, const ArchivePtr& arc, bool highestLodOnly, bool skipTextures) {
    if (!data.success || !sDevice) return;

    InitSharedResources();

    geometry = data.geometry;
    materials = data.materials;
    bones = data.bones;
    textures = data.textures;
    animations = data.animations;
    submeshGroups = data.submeshGroups;

    if (highestLodOnly) {
        uint8_t minGroupId = 255;
        for (const auto& sm : data.geometry.submeshes) {
            if (sm.groupId != 255 && sm.groupId < minGroupId) {
                minGroupId = sm.groupId;
            }
        }
        for (const auto& sm : data.geometry.submeshes) {
            if (sm.groupId == minGroupId || sm.groupId == 255) {
                submeshes.push_back(sm);
            }
        }
    } else {
        submeshes = data.geometry.submeshes;
    }

    materialSelectedVariant.assign(materials.size(), 0);
    submeshVisible.assign(submeshes.size(), 1);
    submeshVariantOverride.assign(submeshes.size(), -1);

    mBoundsMin = glm::vec3(FLT_MAX);
    mBoundsMax = glm::vec3(-FLT_MAX);
    for (const auto& v : data.geometry.vertices) {
        mBoundsMin = glm::min(mBoundsMin, v.position);
        mBoundsMax = glm::max(mBoundsMax, v.position);
    }
    if (mBoundsMin.x > mBoundsMax.x) {
        mBoundsMin = glm::vec3(-1.0f);
        mBoundsMax = glm::vec3(1.0f);
    }

    std::set<uint8_t> uniqueGroups;
    for (const auto& sm : submeshes) {
        if (sm.groupId != 255) uniqueGroups.insert(sm.groupId);
    }
    if (!uniqueGroups.empty()) {
        setActiveVariant(*uniqueGroups.begin());
    }

    precomputeBoneData();

    mFallbackWhiteSRV = createFallbackWhite();

    if (skipTextures) {
        mTextureSRVs.resize(data.textures.size(), nullptr);
        for (const auto& tex : data.textures) {
            pendingTexturePaths.push_back(tex.path);
        }
        archiveRef = arc;
        texturesLoaded = false;
    } else {
        loadTextures(data, arc);
        texturesLoaded = true;
    }

    std::vector<RenderVertex> renderVerts;
    renderVerts.reserve(data.geometry.vertices.size());
    for (const auto& v : data.geometry.vertices) {
        RenderVertex rv;
        rv.position = v.position;
        rv.normal = v.normal;
        rv.uv = v.uv1;
        rv.boneWeights = v.boneWeights;
        rv.boneIndices = v.boneIndices;
        renderVerts.push_back(rv);
    }

    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = static_cast<UINT>(renderVerts.size() * sizeof(RenderVertex));
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vInit = {};
    vInit.pSysMem = renderVerts.data();
    sDevice->CreateBuffer(&vbd, &vInit, &mVertexBuffer);

    D3D11_BUFFER_DESC ibd = {};
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = static_cast<UINT>(data.geometry.indices.size() * sizeof(uint32_t));
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA iInit = {};
    iInit.pSysMem = data.geometry.indices.data();
    sDevice->CreateBuffer(&ibd, &iInit, &mIndexBuffer);

    D3D11_BUFFER_DESC cbd = {};
    cbd.ByteWidth = sizeof(M3ConstantBuffer);
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    sDevice->CreateBuffer(&cbd, nullptr, &mConstantBuffer);

    cbd.ByteWidth = sizeof(BoneMatrixBuffer);
    sDevice->CreateBuffer(&cbd, nullptr, &mBoneBuffer);

    cbd.ByteWidth = sizeof(SkeletonCB);
    sDevice->CreateBuffer(&cbd, nullptr, &mSkeletonCB);
}

M3Render::~M3Render() = default;

void M3Render::precomputeBoneData() {
    size_t numBones = bones.size();
    if (numBones == 0) return;

    effectiveBindGlobal.resize(numBones);
    inverseEffectiveBindGlobal.resize(numBones);
    bindLocalScale.resize(numBones, glm::vec3(1.0f));
    bindLocalRotation.resize(numBones, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    bindLocalTranslation.resize(numBones, glm::vec3(0.0f));
    boneAtOrigin.resize(numBones, false);

    for (size_t i = 0; i < numBones; ++i) {
        const auto& bone = bones[i];
        bool isRootBone = (bone.parentId < 0 || bone.parentId >= (int)numBones);
        bool atOrigin = glm::length(glm::vec3(bone.globalMatrix[3])) < 0.001f;
        boneAtOrigin[i] = atOrigin;

        if (atOrigin && !bone.tracks[6].keyframes.empty()) {
            glm::vec3 track6Pos = bone.tracks[6].keyframes[0].translation;
            glm::mat4 localT = glm::translate(glm::mat4(1.0f), track6Pos);
            if (!isRootBone) {
                effectiveBindGlobal[i] = effectiveBindGlobal[bone.parentId] * localT;
            } else {
                effectiveBindGlobal[i] = localT;
            }
        } else {
            effectiveBindGlobal[i] = bone.globalMatrix;
        }

        inverseEffectiveBindGlobal[i] = glm::inverse(effectiveBindGlobal[i]);
    }

    for (size_t i = 0; i < numBones; ++i) {
        const auto& bone = bones[i];
        bool isRootBone = (bone.parentId < 0 || bone.parentId >= (int)numBones);

        glm::mat4 bindLocal;
        if (isRootBone) {
            bindLocal = effectiveBindGlobal[i];
        } else {
            bindLocal = inverseEffectiveBindGlobal[bone.parentId] * effectiveBindGlobal[i];
        }

        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(bindLocal, bindLocalScale[i], bindLocalRotation[i], bindLocalTranslation[i], skew, perspective);
        bindLocalRotation[i] = glm::normalize(bindLocalRotation[i]);
    }
}

void M3Render::loadTextures(const M3ModelData& data, const ArchivePtr& arc) {
    mTextureSRVs.clear();
    mTextureSRVs.reserve(data.textures.size());
    for (const auto& tex : data.textures) {
        auto srv = loadTextureFromArchive(arc, tex.path);
        if (srv) {
            mTextureSRVs.push_back(srv);
        } else {

            mTextureSRVs.push_back(mFallbackWhiteSRV);
            try {
                RecordTextureFailure(modelName, tex.path);
            } catch (...) {

            }
        }
    }
}

ComPtr<ID3D11ShaderResourceView> M3Render::createFallbackWhite() {
    if (!sDevice) return nullptr;



    uint32_t px = 0xFF808080u;
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = 1;
    td.Height = 1;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = &px;
    initData.SysMemPitch = 4;

    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(sDevice->CreateTexture2D(&td, &initData, &tex))) return nullptr;

    ComPtr<ID3D11ShaderResourceView> srv;
    sDevice->CreateShaderResourceView(tex.Get(), nullptr, &srv);
    return srv;
}


static std::wstring NormalizeTexturePath(const std::wstring& path) {
    std::wstring result = path;

    std::replace(result.begin(), result.end(), L'\\', L'/');

    while (!result.empty() && result[0] == L'/') {
        result.erase(0, 1);
    }

    std::transform(result.begin(), result.end(), result.begin(), ::towlower);
    return result;
}

ComPtr<ID3D11ShaderResourceView> M3Render::loadTextureFromArchive(const ArchivePtr& arc, const std::string& path, bool* outHasAlpha) {
    if (!arc || path.empty() || !sDevice) return nullptr;


    {
        std::lock_guard<std::mutex> lock(sTextureCacheMutex);
        auto it = sTextureSRVCache.find(path);
        if (it != sTextureSRVCache.end()) {
            return it->second;
        }
    }

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    std::wstring basePath = conv.from_bytes(path);


    std::vector<std::wstring> pathsToTry;


    std::wstring withTex = basePath;
    if (withTex.find(L".tex") == std::wstring::npos) {
        withTex += L".tex";
    }
    pathsToTry.push_back(withTex);


    pathsToTry.push_back(NormalizeTexturePath(withTex));


    std::wstring withBackslash = withTex;
    std::replace(withBackslash.begin(), withBackslash.end(), L'/', L'\\');
    pathsToTry.push_back(withBackslash);


    size_t lastSlash = withTex.find_last_of(L"/\\");
    if (lastSlash != std::wstring::npos) {
        pathsToTry.push_back(withTex.substr(lastSlash + 1));
    }

    // Try with "art/" prefix if not present
    std::wstring normalized = NormalizeTexturePath(withTex);
    if (normalized.find(L"art/") != 0) {
        pathsToTry.push_back(L"art/" + normalized);
    }


    auto entry = arc->getByPath(pathsToTry[0]);

    for (size_t i = 1; i < pathsToTry.size() && !entry; ++i) {
        entry = arc->getByPath(pathsToTry[i]);
    }

    if (!entry) {
        return nullptr;
    }

    std::vector<uint8_t> buffer;
    arc->getFileData(std::dynamic_pointer_cast<FileEntry>(entry), buffer);
    if (buffer.empty()) {
        return nullptr;
    }

    Tex::File tf;
    if (!tf.readFromMemory(buffer.data(), buffer.size())) {
        return nullptr;
    }

    Tex::ImageRGBA img;
    if (!tf.decodeLargestMipToRGBA(img)) {
        return nullptr;
    }

    bool detectedAlpha = false;
    size_t pixelCount = img.rgba.size() / 4;
    size_t step = std::max((size_t)1, pixelCount / 1000);
    for (size_t i = 0; i < pixelCount && !detectedAlpha; i += step) {
        if (img.rgba[i * 4 + 3] < 250) {
            detectedAlpha = true;
        }
    }

    D3D11_TEXTURE2D_DESC td = {};;
    td.Width = img.width;
    td.Height = img.height;
    td.MipLevels = 0;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    ComPtr<ID3D11Texture2D> tex;
    if (FAILED(sDevice->CreateTexture2D(&td, nullptr, &tex))) {
        return nullptr;
    }

    sContext->UpdateSubresource(tex.Get(), 0, nullptr, img.rgba.data(), img.width * 4, 0);

    ComPtr<ID3D11ShaderResourceView> srv;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
    srvd.Format = td.Format;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MostDetailedMip = 0;
    srvd.Texture2D.MipLevels = (UINT)-1;
    if (FAILED(sDevice->CreateShaderResourceView(tex.Get(), &srvd, &srv))) {
        return nullptr;
    }

    sContext->GenerateMips(srv.Get());

    {
        std::lock_guard<std::mutex> lock(sTextureCacheMutex);
        sTextureSRVCache[path] = srv;
    }

    if (outHasAlpha) *outHasAlpha = detectedAlpha;
    return srv;
}

ID3D11ShaderResourceView* M3Render::resolveDiffuseTexture(uint16_t materialId, int variant) const {
    static std::set<std::string> printedWarnings;

    auto warnOnce = [&](const std::string& msg) {
        std::string key = modelName + "|" + msg;
        if (printedWarnings.find(key) == printedWarnings.end()) {
            printedWarnings.insert(key);
            std::cerr << "[M3Render] " << modelName << " - " << msg << std::endl;
        }
    };

    if (materialId >= materials.size()) {
        warnOnce("material ID " + std::to_string(materialId) + " out of range (max " + std::to_string(materials.size()) + ")");
        return mFallbackWhiteSRV.Get();
    }
    const auto& m = materials[materialId];
    if (m.variants.empty()) {
        warnOnce("material " + std::to_string(materialId) + " has no variants");
        return mFallbackWhiteSRV.Get();
    }


    int v = std::clamp(variant, 0, (int)m.variants.size() - 1);
    int idx = m.variants[v].textureIndexA;


    if (idx < 0) {
        for (size_t i = 0; i < m.variants.size(); ++i) {
            if (m.variants[i].textureIndexA >= 0) {
                idx = m.variants[i].textureIndexA;
                break;
            }
        }
    }

    if (idx >= 0 && idx < (int)mTextureSRVs.size()) {
        if (mTextureSRVs[idx]) {
            return mTextureSRVs[idx].Get();
        }
        warnOnce("texture slot " + std::to_string(idx) + " is null (not loaded yet?)");
        return mFallbackWhiteSRV.Get();
    }


    if (idx >= (int)mTextureSRVs.size()) {
        warnOnce("texture index " + std::to_string(idx) + " out of range (max " + std::to_string(mTextureSRVs.size()) + ")");
    }
    return mFallbackWhiteSRV.Get();
}

bool M3Render::materialUsesAlpha(uint16_t materialId, int variant) const {
    if (materialId >= materials.size()) return false;
    const auto& m = materials[materialId];
    if (m.variants.empty()) return false;

    int v = std::clamp(variant, 0, (int)m.variants.size() - 1);
    int idx = m.variants[v].textureIndexA;

    if (idx < 0) {
        for (size_t i = 0; i < m.variants.size(); ++i) {
            if (m.variants[i].textureIndexA >= 0) {
                idx = m.variants[i].textureIndexA;
                break;
            }
        }
    }

    if (idx >= 0 && idx < (int)textures.size()) {
        return textures[idx].hasAlpha;
    }
    return false;
}

void M3Render::render(const XMMATRIX& view, const XMMATRIX& proj) {
    render(view, proj, XMMatrixIdentity(), -1);
}

void M3Render::render(const XMMATRIX& view, const XMMATRIX& proj, const XMMATRIX& model) {
    render(view, proj, model, -1);
}

void M3Render::render(const XMMATRIX& view, const XMMATRIX& proj, const XMMATRIX& model, int overrideVariant) {
    if (!sSharedVS || !mVertexBuffer || !sContext) return;

    sContext->RSSetState(sRasterState.Get());
    sContext->OMSetDepthStencilState(sDepthState.Get(), 0);
    float blendFactor[4] = {0, 0, 0, 0};
    sContext->OMSetBlendState(sBlendState.Get(), blendFactor, 0xFFFFFFFF);

    bool useSkinning = (playingAnimation >= 0 && !boneMatrices.empty());

    M3ConstantBuffer cb;
    cb.model = model;
    cb.view = view;
    cb.projection = proj;
    cb.highlightColor = XMFLOAT3(mHighlightR, mHighlightG, mHighlightB);
    cb.highlightMix = mHighlightMix;
    cb.useSkinning = useSkinning ? 1 : 0;

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(sContext->Map(mConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        memcpy(mapped.pData, &cb, sizeof(cb));
        sContext->Unmap(mConstantBuffer.Get(), 0);
    }

    if (useSkinning && mBoneBuffer) {
        BoneMatrixBuffer bb = {};
        size_t count = std::min(boneMatrices.size(), (size_t)200);
        for (size_t i = 0; i < count; ++i) {
            bb.bones[i] = boneMatrices[i];
        }
        if (SUCCEEDED(sContext->Map(mBoneBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            memcpy(mapped.pData, &bb, sizeof(bb));
            sContext->Unmap(mBoneBuffer.Get(), 0);
        }
    }

    sContext->IASetInputLayout(sSharedLayout.Get());
    UINT stride = sizeof(RenderVertex);
    UINT offset = 0;
    sContext->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), &stride, &offset);
    sContext->IASetIndexBuffer(mIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    sContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    sContext->VSSetShader(sSharedVS.Get(), nullptr, 0);
    sContext->PSSetShader(sSharedPS.Get(), nullptr, 0);

    ID3D11Buffer* cbs[] = { mConstantBuffer.Get(), mBoneBuffer.Get() };
    sContext->VSSetConstantBuffers(0, 2, cbs);
    sContext->PSSetConstantBuffers(0, 1, cbs);
    sContext->PSSetSamplers(0, 1, sSharedSampler.GetAddressOf());

    for (size_t i = 0; i < submeshes.size(); ++i) {
        if (i < submeshVisible.size() && submeshVisible[i] == 0) continue;
        const auto& sm = submeshes[i];


        int variant = 0;
        if (overrideVariant >= 0) {
            variant = overrideVariant;
        } else if (i < submeshVariantOverride.size() && submeshVariantOverride[i] >= 0) {
            variant = submeshVariantOverride[i];
        } else if (sm.materialID < materialSelectedVariant.size()) {
            variant = materialSelectedVariant[sm.materialID];
        }

        bool useAlpha = materialUsesAlpha(sm.materialID, variant);
        if (useAlpha) {
            sContext->OMSetBlendState(sAlphaBlendState.Get(), blendFactor, 0xFFFFFFFF);
        } else {
            sContext->OMSetBlendState(sBlendState.Get(), blendFactor, 0xFFFFFFFF);
        }

        ID3D11ShaderResourceView* srv = resolveDiffuseTexture(sm.materialID, variant);
        if (srv) {
            sContext->PSSetShaderResources(0, 1, &srv);
        } else {

            ID3D11ShaderResourceView* nullSRV = nullptr;
            sContext->PSSetShaderResources(0, 1, &nullSRV);
        }

        if ((int)i == selectedSubmesh) {
            cb.highlightColor = XMFLOAT3(0.3f, 1.0f, 0.3f);
            cb.highlightMix = 0.4f;
            if (SUCCEEDED(sContext->Map(mConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
                memcpy(mapped.pData, &cb, sizeof(cb));
                sContext->Unmap(mConstantBuffer.Get(), 0);
            }
        }

        sContext->DrawIndexed(sm.indexCount, sm.startIndex, sm.startVertex);

        if ((int)i == selectedSubmesh) {
            cb.highlightColor = XMFLOAT3(0, 0, 0);
            cb.highlightMix = 0.0f;
        }
    }
}

void M3Render::renderGlm(const glm::mat4& view, const glm::mat4& proj, const glm::mat4& model) {
    render(GlmToXM(view), GlmToXM(proj), GlmToXM(model));
}

size_t M3Render::getSubmeshCount() const { return submeshes.size(); }
const M3Submesh& M3Render::getSubmesh(size_t i) const { return submeshes[i]; }
size_t M3Render::getMaterialCount() const { return materials.size(); }

size_t M3Render::getMaterialVariantCount(size_t materialId) const {
    if (materialId >= materials.size()) return 0;
    return materials[materialId].variants.size();
}

int M3Render::getMaterialSelectedVariant(size_t materialId) const {
    if (materialId >= materialSelectedVariant.size()) return 0;
    return materialSelectedVariant[materialId];
}

void M3Render::setMaterialSelectedVariant(size_t materialId, int variant) {
    if (materialId >= materialSelectedVariant.size()) return;
    int maxV = (int)getMaterialVariantCount(materialId);
    if (maxV <= 0) { materialSelectedVariant[materialId] = 0; return; }
    materialSelectedVariant[materialId] = std::clamp(variant, 0, maxV - 1);
}

bool M3Render::getSubmeshVisible(size_t submeshId) const {
    if (submeshId >= submeshVisible.size()) return true;
    return submeshVisible[submeshId] != 0;
}

void M3Render::setSubmeshVisible(size_t submeshId, bool v) {
    if (submeshId >= submeshVisible.size()) return;
    submeshVisible[submeshId] = v ? 1 : 0;
}

int M3Render::getSubmeshVariantOverride(size_t submeshId) const {
    if (submeshId >= submeshVariantOverride.size()) return -1;
    return submeshVariantOverride[submeshId];
}

void M3Render::setSubmeshVariantOverride(size_t submeshId, int variantOrMinus1) {
    if (submeshId >= submeshVariantOverride.size()) return;
    submeshVariantOverride[submeshId] = variantOrMinus1;
}

void M3Render::setActiveVariant(int variantIndex) {
    activeVariant = variantIndex;
    for (size_t i = 0; i < submeshes.size(); ++i) {
        uint8_t gid = submeshes[i].groupId;
        if (gid == 255 || variantIndex < 0) {
            submeshVisible[i] = 1;
        } else {
            submeshVisible[i] = (gid == (uint8_t)variantIndex) ? 1 : 0;
        }
    }
}

void M3Render::playAnimation(int index) {
    if (index < 0 || index >= (int)animations.size()) {
        stopAnimation();
        return;
    }
    playingAnimation = index;
    animationTime = 0.0f;
    animationPaused = false;
    updateAnimation(0.0f);
}

void M3Render::stopAnimation() {
    playingAnimation = -1;
    animationTime = 0.0f;
    animationPaused = false;
    boneMatrices.clear();
}

void M3Render::pauseAnimation() { animationPaused = true; }
void M3Render::resumeAnimation() { animationPaused = false; }

float M3Render::getAnimationDuration() const {
    if (playingAnimation < 0 || playingAnimation >= (int)animations.size()) return 0.0f;
    const auto& anim = animations[playingAnimation];
    return (anim.timestampEnd - anim.timestampStart) / 1000.0f;
}

static glm::vec3 interpolateScale(const M3AnimationTrack& track, float timeMs) {
    if (track.keyframes.empty()) return glm::vec3(1.0f);
    if (track.keyframes.size() == 1) return track.keyframes[0].scale;
    if (timeMs <= track.keyframes.front().timestamp) return track.keyframes.front().scale;
    if (timeMs >= track.keyframes.back().timestamp) return track.keyframes.back().scale;

    size_t idx = 0;
    for (size_t i = 0; i < track.keyframes.size() - 1; ++i) {
        if (track.keyframes[i].timestamp <= timeMs && track.keyframes[i + 1].timestamp > timeMs) {
            idx = i;
            break;
        }
    }

    const auto& prev = track.keyframes[idx];
    const auto& next = track.keyframes[idx + 1];
    float denom = (float)(next.timestamp - prev.timestamp);
    if (denom <= 0.0f) return prev.scale;
    float t = (timeMs - prev.timestamp) / denom;
    t = glm::clamp(t, 0.0f, 1.0f);
    return glm::mix(prev.scale, next.scale, t);
}

static glm::quat interpolateRotation(const M3AnimationTrack& track, float timeMs) {
    if (track.keyframes.empty()) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    if (track.keyframes.size() == 1) return track.keyframes[0].rotation;
    if (timeMs <= track.keyframes.front().timestamp) return track.keyframes.front().rotation;
    if (timeMs >= track.keyframes.back().timestamp) return track.keyframes.back().rotation;

    size_t idx = 0;
    for (size_t i = 0; i < track.keyframes.size() - 1; ++i) {
        if (track.keyframes[i].timestamp <= timeMs && track.keyframes[i + 1].timestamp > timeMs) {
            idx = i;
            break;
        }
    }

    const auto& prev = track.keyframes[idx];
    const auto& next = track.keyframes[idx + 1];
    float denom = (float)(next.timestamp - prev.timestamp);
    if (denom <= 0.0f) return prev.rotation;
    float t = (timeMs - prev.timestamp) / denom;
    t = glm::clamp(t, 0.0f, 1.0f);

    glm::quat q0 = glm::normalize(prev.rotation);
    glm::quat q1 = glm::normalize(next.rotation);
    if (glm::dot(q0, q1) < 0.0f) {
        q1 = -q1;
    }
    return glm::slerp(q0, q1, t);
}

static glm::vec3 interpolateTranslation(const M3AnimationTrack& track, float timeMs) {
    if (track.keyframes.empty()) return glm::vec3(0.0f);
    if (track.keyframes.size() == 1) return track.keyframes[0].translation;
    if (timeMs <= track.keyframes.front().timestamp) return track.keyframes.front().translation;
    if (timeMs >= track.keyframes.back().timestamp) return track.keyframes.back().translation;

    size_t idx = 0;
    for (size_t i = 0; i < track.keyframes.size() - 1; ++i) {
        if (track.keyframes[i].timestamp <= timeMs && track.keyframes[i + 1].timestamp > timeMs) {
            idx = i;
            break;
        }
    }

    const auto& prev = track.keyframes[idx];
    const auto& next = track.keyframes[idx + 1];
    float denom = (float)(next.timestamp - prev.timestamp);
    if (denom <= 0.0f) return prev.translation;
    float t = (timeMs - prev.timestamp) / denom;
    t = glm::clamp(t, 0.0f, 1.0f);
    return glm::mix(prev.translation, next.translation, t);
}

void M3Render::updateAnimation(float deltaTime) {
    if (playingAnimation < 0 || playingAnimation >= (int)animations.size()) {
        boneMatrices.clear();
        return;
    }

    const auto& anim = animations[playingAnimation];
    float duration = (anim.timestampEnd - anim.timestampStart) / 1000.0f;

    if (duration > 0.0f && !animationPaused) {
        animationTime += deltaTime;
        if (animationTime >= duration) {
            animationTime = std::fmod(animationTime, duration);
        }
    }

    float currentTimeMs = anim.timestampStart + animationTime * 1000.0f;

    size_t numBones = bones.size();
    boneMatrices.resize(numBones);
    worldTransforms.resize(numBones);

    for (size_t i = 0; i < numBones; ++i) {
        const auto& bone = bones[i];
        bool isRoot = (bone.parentId < 0 || bone.parentId >= (int)numBones);

        glm::vec3 scale = bindLocalScale[i];
        glm::quat rotation = bindLocalRotation[i];
        glm::vec3 translation = bindLocalTranslation[i];

        for (int t = 0; t <= 2; ++t) {
            if (!bone.tracks[t].keyframes.empty()) {
                scale = interpolateScale(bone.tracks[t], currentTimeMs);
                break;
            }
        }

        for (int t = 4; t <= 5; ++t) {
            if (!bone.tracks[t].keyframes.empty()) {
                rotation = interpolateRotation(bone.tracks[t], currentTimeMs);
                break;
            }
        }

        if (boneAtOrigin[i] && !bone.tracks[6].keyframes.empty()) {
            translation = interpolateTranslation(bone.tracks[6], currentTimeMs);
        }

        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
        glm::mat4 R = glm::mat4_cast(glm::normalize(rotation));
        glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);
        glm::mat4 localTransform = T * R * S;

        if (isRoot) {
            worldTransforms[i] = localTransform;
        } else {
            worldTransforms[i] = worldTransforms[bone.parentId] * localTransform;
        }

        boneMatrices[i] = GlmToXM(worldTransforms[i] * glm::inverse(bone.globalMatrix));
    }
}

void M3Render::renderSkeleton(const XMMATRIX& view, const XMMATRIX& proj) {
    if (!showSkeleton || bones.empty() || !sSkeletonVS || !sContext || !sDevice) return;

    struct SkeletonVertex {
        glm::vec3 pos;
        glm::vec3 color;
    };

    std::vector<SkeletonVertex> verts;
    verts.reserve(bones.size() * 8);

    std::vector<glm::vec3> boneWorldPos(bones.size());
    for (size_t i = 0; i < bones.size(); ++i) {
        boneWorldPos[i] = glm::vec3(effectiveBindGlobal[i][3]);
    }

    for (size_t i = 0; i < bones.size(); ++i) {
        const auto& bone = bones[i];
        glm::vec3 bonePos = boneWorldPos[i];
        bool isSelected = (selectedBone == static_cast<int>(i));
        float ptSize = isSelected ? 0.06f : 0.02f;
        glm::vec3 jointColor = isSelected ? glm::vec3(1.0f, 0.3f, 0.0f) : glm::vec3(1.0f, 1.0f, 0.0f);

        verts.push_back({bonePos + glm::vec3(-ptSize, 0, 0), jointColor});
        verts.push_back({bonePos + glm::vec3(ptSize, 0, 0), jointColor});
        verts.push_back({bonePos + glm::vec3(0, -ptSize, 0), jointColor});
        verts.push_back({bonePos + glm::vec3(0, ptSize, 0), jointColor});
        verts.push_back({bonePos + glm::vec3(0, 0, -ptSize), jointColor});
        verts.push_back({bonePos + glm::vec3(0, 0, ptSize), jointColor});

        if (bone.parentId >= 0 && bone.parentId < (int)bones.size()) {
            glm::vec3 parentPos = boneWorldPos[bone.parentId];
            bool parentSelected = (selectedBone == bone.parentId);
            glm::vec3 boneColor = (isSelected || parentSelected) ? glm::vec3(1.0f, 0.5f, 0.0f) : glm::vec3(0.0f, 1.0f, 1.0f);
            verts.push_back({bonePos, boneColor});
            verts.push_back({parentPos, boneColor});
        }
    }

    if (verts.empty()) return;

    if (!mSkeletonVB || verts.size() * sizeof(SkeletonVertex) > mSkeletonVBSize) {
        mSkeletonVBSize = verts.size() * sizeof(SkeletonVertex) * 2;
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = static_cast<UINT>(mSkeletonVBSize);
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        sDevice->CreateBuffer(&bd, nullptr, &mSkeletonVB);
    }

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(sContext->Map(mSkeletonVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        memcpy(mapped.pData, verts.data(), verts.size() * sizeof(SkeletonVertex));
        sContext->Unmap(mSkeletonVB.Get(), 0);
    }

    SkeletonCB scb;
    scb.view = view;
    scb.projection = proj;
    if (SUCCEEDED(sContext->Map(mSkeletonCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        memcpy(mapped.pData, &scb, sizeof(scb));
        sContext->Unmap(mSkeletonCB.Get(), 0);
    }

    sContext->IASetInputLayout(sSkeletonLayout.Get());
    UINT stride = sizeof(SkeletonVertex);
    UINT offset = 0;
    sContext->IASetVertexBuffers(0, 1, mSkeletonVB.GetAddressOf(), &stride, &offset);
    sContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    sContext->VSSetShader(sSkeletonVS.Get(), nullptr, 0);
    sContext->PSSetShader(sSkeletonPS.Get(), nullptr, 0);
    sContext->VSSetConstantBuffers(0, 1, mSkeletonCB.GetAddressOf());
    sContext->Draw(static_cast<UINT>(verts.size()), 0);
}

static bool rayTriangleIntersect(const glm::vec3& orig, const glm::vec3& dir,
    const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, float& t) {
    const float EPSILON = 0.0000001f;
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = glm::cross(dir, edge2);
    float a = glm::dot(edge1, h);
    if (a > -EPSILON && a < EPSILON) return false;
    float f = 1.0f / a;
    glm::vec3 s = orig - v0;
    float u = f * glm::dot(s, h);
    if (u < 0.0f || u > 1.0f) return false;
    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(dir, q);
    if (v < 0.0f || u + v > 1.0f) return false;
    t = f * glm::dot(edge2, q);
    return t > EPSILON;
}

int M3Render::rayPickSubmesh(const XMFLOAT3& rayOrigin, const XMFLOAT3& rayDir) const {
    glm::vec3 orig(rayOrigin.x, rayOrigin.y, rayOrigin.z);
    glm::vec3 dir(rayDir.x, rayDir.y, rayDir.z);

    float closestT = FLT_MAX;
    int closestSubmesh = -1;
    const auto& verts = geometry.vertices;
    const auto& inds = geometry.indices;

    for (size_t si = 0; si < submeshes.size(); ++si) {
        if (si < submeshVisible.size() && submeshVisible[si] == 0) continue;
        const auto& sm = submeshes[si];
        for (uint32_t i = 0; i + 2 < sm.indexCount; i += 3) {
            uint32_t i0 = inds[sm.startIndex + i] + sm.startVertex;
            uint32_t i1 = inds[sm.startIndex + i + 1] + sm.startVertex;
            uint32_t i2 = inds[sm.startIndex + i + 2] + sm.startVertex;
            if (i0 >= verts.size() || i1 >= verts.size() || i2 >= verts.size()) continue;
            float t;
            if (rayTriangleIntersect(orig, dir, verts[i0].position, verts[i1].position, verts[i2].position, t)) {
                if (t < closestT) {
                    closestT = t;
                    closestSubmesh = static_cast<int>(si);
                }
            }
        }
    }
    return closestSubmesh;
}

int M3Render::rayPickBone(const XMFLOAT3& rayOrigin, const XMFLOAT3& rayDir) const {
    if (bones.empty()) return -1;

    glm::vec3 orig(rayOrigin.x, rayOrigin.y, rayOrigin.z);
    glm::vec3 dir(rayDir.x, rayDir.y, rayDir.z);

    int closestBone = -1;
    float closestDist = FLT_MAX;
    float pickRadius = 0.15f;

    for (size_t i = 0; i < bones.size(); ++i) {
        glm::vec3 bonePos = glm::vec3(effectiveBindGlobal[i][3]);
        glm::vec3 toPoint = bonePos - orig;
        float t = glm::dot(toPoint, dir);
        if (t < 0) continue;
        glm::vec3 closestPoint = orig + dir * t;
        float dist = glm::length(bonePos - closestPoint);
        if (dist < pickRadius && t < closestDist) {
            closestDist = t;
            closestBone = static_cast<int>(i);
        }
    }

    return closestBone;
}

void M3Render::queueTexturesForLoading() {
    nextTextureToLoad = 0;
}

void M3Render::setTextureSRV(size_t index, ComPtr<ID3D11ShaderResourceView> srv) {
    if (index < mTextureSRVs.size()) {
        mTextureSRVs[index] = srv;
    }
}

bool M3Render::uploadNextTexture(const ArchivePtr& arc) {
    if (nextTextureToLoad >= pendingTexturePaths.size()) {
        texturesLoaded = true;
        return false;
    }

    const ArchivePtr& archive = arc ? arc : archiveRef;
    if (!archive) return false;

    const std::string& path = pendingTexturePaths[nextTextureToLoad];
    bool hasAlpha = false;
    auto srv = loadTextureFromArchive(archive, path, &hasAlpha);

    if (nextTextureToLoad < mTextureSRVs.size()) {
        if (srv) {
            mTextureSRVs[nextTextureToLoad] = srv;
            if (nextTextureToLoad < textures.size()) {
                textures[nextTextureToLoad].hasAlpha = hasAlpha;
            }
        } else {

            mTextureSRVs[nextTextureToLoad] = mFallbackWhiteSRV;


            try {
                RecordTextureFailure(modelName, path);
            } catch (...) {

            }
        }
    }

    nextTextureToLoad++;

    if (nextTextureToLoad >= pendingTexturePaths.size()) {
        texturesLoaded = true;
    }

    return true;
}

void M3Render::renderSkeleton(const XMMATRIX& view, const XMMATRIX& proj, const XMMATRIX& model) {
    if (!showSkeleton || bones.empty() || !sSkeletonVS || !sContext || !sDevice) return;

    struct SkeletonVertex {
        glm::vec3 pos;
        glm::vec3 color;
    };

    std::vector<SkeletonVertex> verts;
    verts.reserve(bones.size() * 8);

    std::vector<glm::vec3> boneWorldPos(bones.size());
    for (size_t i = 0; i < bones.size(); ++i) {
        glm::vec4 localPos = glm::vec4(glm::vec3(effectiveBindGlobal[i][3]), 1.0f);
        XMVECTOR pos = XMVectorSet(localPos.x, localPos.y, localPos.z, 1.0f);
        pos = XMVector4Transform(pos, model);
        XMFLOAT4 transformed;
        XMStoreFloat4(&transformed, pos);
        boneWorldPos[i] = glm::vec3(transformed.x, transformed.y, transformed.z);
    }

    for (size_t i = 0; i < bones.size(); ++i) {
        const auto& bone = bones[i];
        glm::vec3 bonePos = boneWorldPos[i];
        bool isSelected = (selectedBone == static_cast<int>(i));
        float ptSize = isSelected ? 0.06f : 0.02f;
        glm::vec3 jointColor = isSelected ? glm::vec3(1.0f, 0.3f, 0.0f) : glm::vec3(1.0f, 1.0f, 0.0f);

        verts.push_back({bonePos + glm::vec3(-ptSize, 0, 0), jointColor});
        verts.push_back({bonePos + glm::vec3(ptSize, 0, 0), jointColor});
        verts.push_back({bonePos + glm::vec3(0, -ptSize, 0), jointColor});
        verts.push_back({bonePos + glm::vec3(0, ptSize, 0), jointColor});
        verts.push_back({bonePos + glm::vec3(0, 0, -ptSize), jointColor});
        verts.push_back({bonePos + glm::vec3(0, 0, ptSize), jointColor});

        if (bone.parentId >= 0 && bone.parentId < (int)bones.size()) {
            glm::vec3 parentPos = boneWorldPos[bone.parentId];
            bool parentSelected = (selectedBone == bone.parentId);
            glm::vec3 boneColor = (isSelected || parentSelected) ? glm::vec3(1.0f, 0.5f, 0.0f) : glm::vec3(0.0f, 1.0f, 1.0f);
            verts.push_back({bonePos, boneColor});
            verts.push_back({parentPos, boneColor});
        }
    }

    if (verts.empty()) return;

    if (!mSkeletonVB || verts.size() * sizeof(SkeletonVertex) > mSkeletonVBSize) {
        mSkeletonVBSize = verts.size() * sizeof(SkeletonVertex) * 2;
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = static_cast<UINT>(mSkeletonVBSize);
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        sDevice->CreateBuffer(&bd, nullptr, &mSkeletonVB);
    }

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(sContext->Map(mSkeletonVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        memcpy(mapped.pData, verts.data(), verts.size() * sizeof(SkeletonVertex));
        sContext->Unmap(mSkeletonVB.Get(), 0);
    }

    SkeletonCB scb;
    scb.view = view;
    scb.projection = proj;
    if (SUCCEEDED(sContext->Map(mSkeletonCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        memcpy(mapped.pData, &scb, sizeof(scb));
        sContext->Unmap(mSkeletonCB.Get(), 0);
    }

    sContext->IASetInputLayout(sSkeletonLayout.Get());
    UINT stride = sizeof(SkeletonVertex);
    UINT offset = 0;
    sContext->IASetVertexBuffers(0, 1, mSkeletonVB.GetAddressOf(), &stride, &offset);
    sContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    sContext->VSSetShader(sSkeletonVS.Get(), nullptr, 0);
    sContext->PSSetShader(sSkeletonPS.Get(), nullptr, 0);
    sContext->VSSetConstantBuffers(0, 1, mSkeletonCB.GetAddressOf());
    sContext->Draw(static_cast<UINT>(verts.size()), 0);
}

int M3Render::rayPick(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::mat4& modelMatrix, float& outDist) const {
    glm::mat4 invModel = glm::inverse(modelMatrix);
    glm::vec4 localOrigin4 = invModel * glm::vec4(rayOrigin, 1.0f);
    glm::vec4 localDir4 = invModel * glm::vec4(rayDir, 0.0f);
    glm::vec3 localOrigin(localOrigin4);
    glm::vec3 localDir = glm::normalize(glm::vec3(localDir4));

    float closestT = std::numeric_limits<float>::max();
    int hitSubmesh = -1;

    const auto& verts = geometry.vertices;
    const auto& indices = geometry.indices;

    for (size_t smIdx = 0; smIdx < submeshes.size(); ++smIdx) {
        if (!submeshVisible[smIdx]) continue;

        const auto& sm = submeshes[smIdx];
        uint32_t startIdx = sm.startIndex;
        uint32_t endIdx = startIdx + sm.indexCount;

        for (uint32_t i = startIdx; i + 2 < endIdx; i += 3) {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];

            if (i0 >= verts.size() || i1 >= verts.size() || i2 >= verts.size()) continue;

            const glm::vec3& v0 = verts[i0].position;
            const glm::vec3& v1 = verts[i1].position;
            const glm::vec3& v2 = verts[i2].position;

            float t = 0.0f;
            const float EPSILON = 1e-8f;
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 h = glm::cross(localDir, edge2);
            float a = glm::dot(edge1, h);

            if (a > -EPSILON && a < EPSILON) continue;

            float f = 1.0f / a;
            glm::vec3 s = localOrigin - v0;
            float u = f * glm::dot(s, h);
            if (u < 0.0f || u > 1.0f) continue;

            glm::vec3 q = glm::cross(s, edge1);
            float v = f * glm::dot(localDir, q);
            if (v < 0.0f || u + v > 1.0f) continue;

            t = f * glm::dot(edge2, q);
            if (t > EPSILON && t < closestT) {
                closestT = t;
                hitSubmesh = static_cast<int>(smIdx);
            }
        }
    }

    if (hitSubmesh >= 0) {
        glm::vec3 localHit = localOrigin + localDir * closestT;
        glm::vec4 worldHit = modelMatrix * glm::vec4(localHit, 1.0f);
        outDist = glm::length(glm::vec3(worldHit) - rayOrigin);
    }

    return hitSubmesh;
}