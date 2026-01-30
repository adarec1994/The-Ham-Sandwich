#include "UI_RenderWorld.h"
#include "../Area/AreaRender.h"
#include "../Area/AreaFile.h"
#include "../Area/TerrainShader.h"
#include "../models/M3Render.h"
#include "../Skybox/Sky_Manager.h"
#include "UI_Globals.h"
#include "UI_Selection.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <fstream>

using Microsoft::WRL::ComPtr;

extern ID3D11Device* gDevice;
extern ID3D11DeviceContext* gContext;

static AreaRender gAreaRender;
static bool gAreaRenderInitialized = false;

static inline glm::vec3 ToGlm(const DirectX::XMFLOAT3& v)
{
    return glm::vec3(v.x, v.y, v.z);
}

static inline DirectX::XMFLOAT3 ToDX3(const glm::vec3& v)
{
    return DirectX::XMFLOAT3(v.x, v.y, v.z);
}

static inline DirectX::XMMATRIX ToDXMatrix(const glm::mat4& m)
{
    return DirectX::XMMATRIX(
        m[0][0], m[1][0], m[2][0], m[3][0],
        m[0][1], m[1][1], m[2][1], m[3][1],
        m[0][2], m[1][2], m[2][2], m[3][2],
        m[0][3], m[1][3], m[2][3], m[3][3]
    );
}

static ComPtr<ID3D11Buffer> gHighlightVB;
static ComPtr<ID3D11VertexShader> gHighlightVS;
static ComPtr<ID3D11PixelShader> gHighlightPS;
static ComPtr<ID3D11InputLayout> gHighlightInputLayout;
static ComPtr<ID3D11Buffer> gHighlightCB;
static ComPtr<ID3D11RasterizerState> gHighlightRasterState;
static ComPtr<ID3D11DepthStencilState> gHighlightDepthState;

struct HighlightConstants
{
    DirectX::XMFLOAT4X4 mvp;
    DirectX::XMFLOAT4 color;
};

static const char* highlightShaderSource = R"(
cbuffer HighlightCB : register(b0)
{
    float4x4 mvp;
    float4 color;
};

struct VS_INPUT
{
    float3 pos : POSITION;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
};

PS_INPUT VS_Main(VS_INPUT input)
{
    PS_INPUT output;
    output.pos = mul(float4(input.pos, 1.0), mvp);
    return output;
}

float4 PS_Main(PS_INPUT input) : SV_TARGET
{
    return color;
}
)";

static void InitHighlightShader()
{
    if (gHighlightVS || !gDevice) return;

    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3DCompile(highlightShaderSource, strlen(highlightShaderSource), nullptr, nullptr, nullptr,
                            "VS_Main", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) return;

    hr = D3DCompile(highlightShaderSource, strlen(highlightShaderSource), nullptr, nullptr, nullptr,
                    "PS_Main", "ps_5_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr)) return;

    gDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &gHighlightVS);
    gDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &gHighlightPS);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    gDevice->CreateInputLayout(layout, 1, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &gHighlightInputLayout);

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(HighlightConstants);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    gDevice->CreateBuffer(&cbDesc, nullptr, &gHighlightCB);

    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = 24 * 3 * sizeof(float);
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    gDevice->CreateBuffer(&vbDesc, nullptr, &gHighlightVB);

    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthClipEnable = TRUE;
    gDevice->CreateRasterizerState(&rasterDesc, &gHighlightRasterState);

    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    gDevice->CreateDepthStencilState(&dsDesc, &gHighlightDepthState);
}

static void RenderAreaHighlight(const AreaFilePtr& area, const glm::mat4& view, const glm::mat4& projection)
{
    if (!area || !gContext) return;

    InitHighlightShader();
    if (!gHighlightVS) return;

    glm::vec3 worldOffset = ToGlm(area->getWorldOffset());
    glm::vec3 minB = ToGlm(area->getMinBounds()) + worldOffset;
    glm::vec3 maxB = ToGlm(area->getMaxBounds()) + worldOffset;

    float vertices[] = {
        minB.x, minB.y, minB.z,
        maxB.x, minB.y, minB.z,
        maxB.x, minB.y, minB.z,
        maxB.x, minB.y, maxB.z,
        maxB.x, minB.y, maxB.z,
        minB.x, minB.y, maxB.z,
        minB.x, minB.y, maxB.z,
        minB.x, minB.y, minB.z,

        minB.x, maxB.y, minB.z,
        maxB.x, maxB.y, minB.z,
        maxB.x, maxB.y, minB.z,
        maxB.x, maxB.y, maxB.z,
        maxB.x, maxB.y, maxB.z,
        minB.x, maxB.y, maxB.z,
        minB.x, maxB.y, maxB.z,
        minB.x, maxB.y, minB.z,

        minB.x, minB.y, minB.z,
        minB.x, maxB.y, minB.z,
        maxB.x, minB.y, minB.z,
        maxB.x, maxB.y, minB.z,
        maxB.x, minB.y, maxB.z,
        maxB.x, maxB.y, maxB.z,
        minB.x, minB.y, maxB.z,
        minB.x, maxB.y, maxB.z,
    };

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(gContext->Map(gHighlightVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, vertices, sizeof(vertices));
        gContext->Unmap(gHighlightVB.Get(), 0);
    }

    glm::mat4 mvp = projection * view;
    DirectX::XMFLOAT4X4 mvpDX;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            mvpDX.m[i][j] = mvp[j][i];

    HighlightConstants cb;
    cb.mvp = mvpDX;
    cb.color = { 0.0f, 1.0f, 0.0f, 1.0f };

    if (SUCCEEDED(gContext->Map(gHighlightCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        memcpy(mapped.pData, &cb, sizeof(cb));
        gContext->Unmap(gHighlightCB.Get(), 0);
    }

    UINT stride = 3 * sizeof(float);
    UINT offset = 0;
    ID3D11Buffer* vb = gHighlightVB.Get();
    gContext->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    gContext->IASetInputLayout(gHighlightInputLayout.Get());
    gContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    gContext->VSSetShader(gHighlightVS.Get(), nullptr, 0);
    gContext->PSSetShader(gHighlightPS.Get(), nullptr, 0);
    ID3D11Buffer* cbs[] = { gHighlightCB.Get() };
    gContext->VSSetConstantBuffers(0, 1, cbs);
    gContext->PSSetConstantBuffers(0, 1, cbs);

    gContext->RSSetState(gHighlightRasterState.Get());
    gContext->OMSetDepthStencilState(gHighlightDepthState.Get(), 0);

    gContext->Draw(24, 0);
}

static void HandleModelPicking(AppState& state)
{
    M3Render* render = state.m3Render.get();
    if (!render) return;
    if (ImGui::GetIO().WantCaptureMouse) return;
    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return;

    ImVec2 mousePos = ImGui::GetMousePos();
    ImGuiViewport* vp = ImGui::GetMainViewport();

    float ndcX = (2.0f * (mousePos.x - vp->Pos.x) / vp->Size.x) - 1.0f;
    float ndcY = 1.0f - (2.0f * (mousePos.y - vp->Pos.y) / vp->Size.y);

    glm::mat4 invProj = glm::inverse(gProjMatrix);
    glm::mat4 invView = glm::inverse(gViewMatrix);

    glm::vec4 rayClip(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 rayEye = invProj * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    glm::vec3 rayWorld = glm::normalize(glm::vec3(invView * rayEye));
    glm::vec3 rayOrigin = glm::vec3(invView[3]);

    DirectX::XMFLOAT3 dxOrigin = ToDX3(rayOrigin);
    DirectX::XMFLOAT3 dxDir = ToDX3(rayWorld);

    if (render->getShowSkeleton())
    {
        int hit = render->rayPickBone(dxOrigin, dxDir);
        render->setSelectedBone(hit);
        render->setSelectedSubmesh(-1);
    }
    else
    {
        int hit = render->rayPickSubmesh(dxOrigin, dxDir);
        render->setSelectedSubmesh(hit);
        render->setSelectedBone(-1);
    }
}

void HandleAreaPicking(AppState& state)
{
    if (gLoadedModel)
    {
        HandleModelPicking(state);
        return;
    }

    if (gLoadedAreas.empty()) return;
    if (ImGui::GetIO().WantCaptureMouse) return;
    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return;

    CheckAreaSelection(state);
}

void RenderAreas(const AppState& state, int display_w, int display_h)
{
    if (display_w <= 0 || display_h <= 0) return;
    if (!gContext) return;

    D3D11_VIEWPORT vp = {};
    vp.Width = static_cast<float>(display_w);
    vp.Height = static_cast<float>(display_h);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    gContext->RSSetViewports(1, &vp);

    const glm::mat4 view = glm::lookAt(
        state.camera.Position,
        state.camera.Position + state.camera.Front,
        state.camera.Up
    );

    const glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(display_w) / static_cast<float>(display_h),
        0.1f,
        100000.0f
    );

    gViewMatrix = view;
    gProjMatrix = projection;

    DirectX::XMMATRIX dxView = ToDXMatrix(view);
    DirectX::XMMATRIX dxProj = ToDXMatrix(projection);

    if (gLoadedModel)
    {
        gLoadedModel->updateAnimation(ImGui::GetIO().DeltaTime);
        gLoadedModel->render(dxView, dxProj);
        gLoadedModel->renderSkeleton(dxView, dxProj);
    }
    else if (!gLoadedAreas.empty())
    {
        if (!gAreaRenderInitialized && gDevice)
        {
            gAreaRender.init(gDevice);
            gAreaRenderInitialized = true;
        }

        if (gAreaRender.isInitialized())
        {
            Sky::Manager::Instance().render(view, projection, state.camera.Position);

            gAreaRender.bind(gContext);

            // DEBUG: Log gLoadedAreas size once after load
            static size_t sLastLoggedSize = 999999;
            if (gLoadedAreas.size() != sLastLoggedSize)
            {
                std::ofstream dbg("area_debug.log", std::ios::app);
                if (dbg.is_open())
                {
                    dbg << "[RenderAreas] gLoadedAreas.size() changed to " << gLoadedAreas.size() << "\n";
                    dbg.close();
                }
                sLastLoggedSize = gLoadedAreas.size();
            }

            // DEBUG: Only render first N areas to test if multi-area is the issue
            static constexpr size_t DEBUG_MAX_AREAS = 999; // Set to 1 to test single area, 999 for all
            size_t areasRendered = 0;

            for (size_t i = 0; i < gLoadedAreas.size() && areasRendered < DEBUG_MAX_AREAS; ++i)
            {
                const auto& area = gLoadedAreas[i];
                if (!area) continue;

                // Skip areas with no valid chunks to avoid render issues
                bool hasValidChunks = false;
                for (const auto& chunk : area->getChunks())
                {
                    if (chunk && chunk->isFullyInitialized())
                    {
                        hasValidChunks = true;
                        break;
                    }
                }
                if (!hasValidChunks) continue;

                bool isSelected = (static_cast<int>(i) == gSelectedAreaIndex && gSelectedPropID == 0);
                area->render(gContext, dxView, dxProj, gAreaRender.getConstantBuffer(), isSelected);
                areasRendered++;
            }
        }

        const Prop* selectedProp = nullptr;
        DirectX::XMMATRIX selectedPropModelMatrix = DirectX::XMMatrixIdentity();

        if (gShowProps)
        {
            for (size_t areaIdx = 0; areaIdx < gLoadedAreas.size(); ++areaIdx)
            {
                const auto& area = gLoadedAreas[areaIdx];
                if (!area) continue;

                DirectX::XMFLOAT3 worldOffset = area->getWorldOffset();
                const auto& props = area->getProps();

                for (size_t pi = 0; pi < props.size(); ++pi)
                {
                    const Prop& prop = props[pi];
                    if (!prop.loaded || !prop.render) continue;
                    if (!IsPropVisible(prop.uniqueID)) continue;

                    glm::vec3 pos(
                        prop.position.x + worldOffset.x,
                        prop.position.y + worldOffset.y,
                        prop.position.z + worldOffset.z
                    );

                    glm::mat4 glmModel = glm::mat4(1.0f);
                    glmModel = glm::translate(glmModel, pos);
                    glmModel = glmModel * glm::mat4_cast(prop.rotation);
                    glmModel = glm::scale(glmModel, glm::vec3(prop.scale));

                    DirectX::XMMATRIX model = ToDXMatrix(glmModel);

                    bool isSelected = IsPropSelected(prop.uniqueID);

                    if (isSelected)
                    {
                        selectedProp = &prop;
                        selectedPropModelMatrix = model;

                        prop.render->updateAnimation(ImGui::GetIO().DeltaTime);

                        prop.render->setHighlightColor(0.2f, 1.0f, 0.2f, 0.5f);
                        prop.render->render(dxView, dxProj, model, -1);
                        prop.render->setHighlightColor(0.0f, 0.0f, 0.0f, 0.0f);
                    }
                    else
                    {
                        int variant = prop.unk7 & 0xFF;
                        prop.render->render(dxView, dxProj, model, variant);
                    }
                }
            }
        }

        if (selectedProp && selectedProp->render && selectedProp->render->getShowSkeleton())
        {
            selectedProp->render->renderSkeleton(dxView, dxProj, selectedPropModelMatrix);
        }
    }
}

namespace UI_RenderWorld
{
    void Draw(AppState& state)
    {
        HandleAreaPicking(state);

        ImGuiIO& io = ImGui::GetIO();
        RenderAreas(state, static_cast<int>(io.DisplaySize.x), static_cast<int>(io.DisplaySize.y));
    }
}