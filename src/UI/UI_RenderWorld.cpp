#include "UI_RenderWorld.h"
#include "../Area/AreaRender.h"
#include "../Area/AreaFile.h"
#include "../Area/TerrainShader.h"
#include "../Area/Props.h"
#include "../models/M3Render.h"
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
            gAreaRender.bind(gContext);

            for (size_t i = 0; i < gLoadedAreas.size(); ++i)
            {
                const auto& area = gLoadedAreas[i];
                if (!area) continue;

                bool isSelected = (static_cast<int>(i) == gSelectedAreaIndex && gSelectedPropID == 0);
                AreaChunkRenderPtr highlightChunk = isSelected ? gSelectedChunk : nullptr;
                area->render(gContext, dxView, dxProj, gAreaRender.getConstantBuffer(), highlightChunk);
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