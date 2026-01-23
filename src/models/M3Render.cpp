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
bool M3Render::sShadersInitialized = false;

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
    float3 texColor = diffTexture.Sample(samplerState, input.texCoord).rgb;
    float3 baseColor = texColor * diff;
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

    sShadersInitialized = true;
}

M3Render::M3Render(const M3ModelData& data, const ArchivePtr& arc, bool highestLodOnly) {
    auto startTime = std::chrono::high_resolution_clock::now();

    if (!data.success || !sDevice) return;

    InitSharedResources();

    geometry = data.geometry;
    submeshes = data.geometry.submeshes;
    materials = data.materials;
    bones = data.bones;
    textures = data.textures;
    animations = data.animations;
    submeshGroups = data.submeshGroups;
    materialSelectedVariant.assign(materials.size(), 0);
    submeshVisible.assign(submeshes.size(), 1);
    submeshVariantOverride.assign(submeshes.size(), -1);

    std::set<uint8_t> uniqueGroups;
    for (const auto& sm : submeshes) {
        if (sm.groupId != 255) uniqueGroups.insert(sm.groupId);
    }
    if (!uniqueGroups.empty()) {
        setActiveVariant(*uniqueGroups.begin());
    }

    precomputeBoneData();
    loadTextures(data, arc);
    mFallbackWhiteSRV = createFallbackWhite();

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

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    printf("M3 model loaded in %lld ms\n", duration);
}

M3Render::~M3Render() = default;

void M3Render::precomputeBoneData() {
    size_t numBones = bones.size();
    if (numBones == 0) return;

    effectiveBindGlobal.resize(numBones);
    inverseEffectiveBindGlobal.resize(numBones);
    bindLocalScale.resize(numBones);
    bindLocalRotation.resize(numBones);
    bindLocalTranslation.resize(numBones);

    for (size_t i = 0; i < numBones; ++i) {
        const auto& bone = bones[i];
        glm::vec3 globalPos = glm::vec3(bone.globalMatrix[3]);
        bool boneAtOrigin = glm::length(globalPos) < 0.001f;

        if (boneAtOrigin && !bone.tracks[6].keyframes.empty()) {
            glm::mat4 rotScale = bone.globalMatrix;
            rotScale[3] = glm::vec4(0, 0, 0, 1);
            glm::vec3 track6Pos = bone.tracks[6].keyframes[0].translation;
            effectiveBindGlobal[i] = glm::translate(glm::mat4(1.0f), track6Pos) * rotScale;
        } else {
            effectiveBindGlobal[i] = bone.globalMatrix;
        }

        inverseEffectiveBindGlobal[i] = glm::inverse(effectiveBindGlobal[i]);
    }

    for (size_t i = 0; i < numBones; ++i) {
        const auto& bone = bones[i];
        bool isRoot = (bone.parentId < 0 || bone.parentId >= (int)numBones);

        glm::mat4 bindLocal;
        if (isRoot) {
            bindLocal = effectiveBindGlobal[i];
        } else {
            bindLocal = inverseEffectiveBindGlobal[bone.parentId] * effectiveBindGlobal[i];
        }

        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(bindLocal, bindLocalScale[i], bindLocalRotation[i], bindLocalTranslation[i], skew, perspective);
    }
}

void M3Render::loadTextures(const M3ModelData& data, const ArchivePtr& arc) {
    mTextureSRVs.clear();
    mTextureSRVs.reserve(data.textures.size());
    for (const auto& tex : data.textures) {
        mTextureSRVs.push_back(loadTextureFromArchive(arc, tex.path));
    }
}

ComPtr<ID3D11ShaderResourceView> M3Render::createFallbackWhite() {
    if (!sDevice) return nullptr;

    uint32_t px = 0xFFFFFFFFu;
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

ComPtr<ID3D11ShaderResourceView> M3Render::loadTextureFromArchive(const ArchivePtr& arc, const std::string& path) {
    if (!arc || path.empty() || !sDevice) return nullptr;

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    std::wstring wp = conv.from_bytes(path);
    if (wp.find(L".tex") == std::wstring::npos) wp += L".tex";
    auto entry = arc->getByPath(wp);
    if (!entry) {
        std::replace(wp.begin(), wp.end(), L'/', L'\\');
        entry = arc->getByPath(wp);
    }
    if (!entry) return nullptr;

    std::vector<uint8_t> buffer;
    arc->getFileData(std::dynamic_pointer_cast<FileEntry>(entry), buffer);
    if (buffer.empty()) return nullptr;

    Tex::File tf;
    if (!tf.readFromMemory(buffer.data(), buffer.size())) return nullptr;

    Tex::ImageRGBA img;
    if (!tf.decodeLargestMipToRGBA(img)) return nullptr;

    D3D11_TEXTURE2D_DESC td = {};
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
    if (FAILED(sDevice->CreateTexture2D(&td, nullptr, &tex))) return nullptr;

    if (sContext) {
        sContext->UpdateSubresource(tex.Get(), 0, nullptr, img.rgba.data(), img.width * 4, 0);
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = static_cast<UINT>(-1);

    ComPtr<ID3D11ShaderResourceView> srv;
    if (FAILED(sDevice->CreateShaderResourceView(tex.Get(), &srvDesc, &srv))) return nullptr;

    if (sContext) {
        sContext->GenerateMips(srv.Get());
    }

    return srv;
}

ID3D11ShaderResourceView* M3Render::resolveDiffuseTexture(uint16_t materialId, int variant) const {
    if (materialId >= materials.size()) return mFallbackWhiteSRV.Get();
    const auto& m = materials[materialId];
    if (m.variants.empty()) return mFallbackWhiteSRV.Get();
    int v = std::clamp(variant, 0, (int)m.variants.size() - 1);
    int idx = m.variants[v].textureIndexA;
    if (idx >= 0 && idx < (int)mTextureSRVs.size() && mTextureSRVs[idx]) {
        return mTextureSRVs[idx].Get();
    }
    return mFallbackWhiteSRV.Get();
}

void M3Render::render(const XMMATRIX& view, const XMMATRIX& proj) {
    render(view, proj, XMMatrixIdentity());
}

void M3Render::render(const XMMATRIX& view, const XMMATRIX& proj, const XMMATRIX& model) {
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
    cb.highlightColor = XMFLOAT3(0, 0, 0);
    cb.highlightMix = 0.0f;
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
        if (i < submeshVariantOverride.size() && submeshVariantOverride[i] >= 0) {
            variant = submeshVariantOverride[i];
        } else if (sm.materialID < materialSelectedVariant.size()) {
            variant = materialSelectedVariant[sm.materialID];
        }

        ID3D11ShaderResourceView* srv = resolveDiffuseTexture(sm.materialID, variant);
        sContext->PSSetShaderResources(0, 1, &srv);

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
    size_t idx = 0;
    for (size_t i = 0; i < track.keyframes.size(); ++i) {
        if (track.keyframes[i].timestamp <= timeMs) idx = i;
        else break;
    }
    const auto& prev = track.keyframes[idx];
    if (idx + 1 >= track.keyframes.size()) return prev.scale;
    const auto& next = track.keyframes[idx + 1];
    if (next.timestamp == prev.timestamp) return prev.scale;
    float t = (timeMs - prev.timestamp) / (float)(next.timestamp - prev.timestamp);
    return glm::mix(prev.scale, next.scale, glm::clamp(t, 0.0f, 1.0f));
}

static glm::quat interpolateRotation(const M3AnimationTrack& track, float timeMs) {
    if (track.keyframes.empty()) return glm::quat(1, 0, 0, 0);
    size_t idx = 0;
    for (size_t i = 0; i < track.keyframes.size(); ++i) {
        if (track.keyframes[i].timestamp <= timeMs) idx = i;
        else break;
    }
    const auto& prev = track.keyframes[idx];
    if (idx + 1 >= track.keyframes.size()) return prev.rotation;
    const auto& next = track.keyframes[idx + 1];
    if (next.timestamp == prev.timestamp) return prev.rotation;
    float t = (timeMs - prev.timestamp) / (float)(next.timestamp - prev.timestamp);
    return glm::slerp(prev.rotation, next.rotation, glm::clamp(t, 0.0f, 1.0f));
}

static glm::vec3 interpolateTranslation(const M3AnimationTrack& track, float timeMs) {
    if (track.keyframes.empty()) return glm::vec3(0.0f);
    size_t idx = 0;
    for (size_t i = 0; i < track.keyframes.size(); ++i) {
        if (track.keyframes[i].timestamp <= timeMs) idx = i;
        else break;
    }
    const auto& prev = track.keyframes[idx];
    if (idx + 1 >= track.keyframes.size()) return prev.translation;
    const auto& next = track.keyframes[idx + 1];
    if (next.timestamp == prev.timestamp) return prev.translation;
    float t = (timeMs - prev.timestamp) / (float)(next.timestamp - prev.timestamp);
    return glm::mix(prev.translation, next.translation, glm::clamp(t, 0.0f, 1.0f));
}

static XMMATRIX GlmToXM(const glm::mat4& m) {
    return XMMATRIX(
        m[0][0], m[0][1], m[0][2], m[0][3],
        m[1][0], m[1][1], m[1][2], m[1][3],
        m[2][0], m[2][1], m[2][2], m[2][3],
        m[3][0], m[3][1], m[3][2], m[3][3]
    );
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

        if (!bone.tracks[6].keyframes.empty()) {
            translation = interpolateTranslation(bone.tracks[6], currentTimeMs);
        }

        glm::mat4 localTransform = glm::translate(glm::mat4(1.0f), translation) *
                                   glm::mat4_cast(rotation) *
                                   glm::scale(glm::mat4(1.0f), scale);

        if (isRoot) {
            worldTransforms[i] = localTransform;
        } else {
            worldTransforms[i] = worldTransforms[bone.parentId] * localTransform;
        }

        boneMatrices[i] = GlmToXM(worldTransforms[i] * inverseEffectiveBindGlobal[i]);
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

void M3Render::queueTexturesForLoading() {}
bool M3Render::uploadNextTexture(const ArchivePtr&) { return false; }