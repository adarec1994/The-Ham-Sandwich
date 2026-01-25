#include "UI_Selection.h"
#include "UI_Globals.h"
#include "UI_Utils.h"
#include "../Area/Props.h"
#include "../models/M3Render.h"
#include "../Skybox/Sky_Manager.h"

#include <limits>
#include <string>
#include <vector>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <DirectXMath.h>

#include <imgui.h>
#include <windows.h>

extern HWND gHwnd;

uint32_t gSelectedPropID = 0;
int gSelectedPropAreaIndex = -1;
std::set<uint32_t> gHiddenProps;
std::set<uint32_t> gDeletedProps;
int gSelectedSkyModelIndex = -1;

static bool RayIntersectsAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                              const glm::vec3& boxMin, const glm::vec3& boxMax, float& tOut)
{
    float tMin = -std::numeric_limits<float>::max();
    float tMax = std::numeric_limits<float>::max();

    for (int i = 0; i < 3; ++i)
    {
        if (std::abs(rayDir[i]) < 1e-8f)
        {
            if (rayOrigin[i] < boxMin[i] || rayOrigin[i] > boxMax[i])
                return false;
        }
        else
        {
            float invD = 1.0f / rayDir[i];
            float t0 = (boxMin[i] - rayOrigin[i]) * invD;
            float t1 = (boxMax[i] - rayOrigin[i]) * invD;

            if (invD < 0.0f)
                std::swap(t0, t1);

            tMin = t0 > tMin ? t0 : tMin;
            tMax = t1 < tMax ? t1 : tMax;

            if (tMax < tMin)
                return false;
        }
    }

    if (tMax < 0.0f)
        return false;

    tOut = tMin >= 0.0f ? tMin : tMax;
    return true;
}

void CheckAreaSelection(AppState& state)
{
    ImGuiIO& io = ImGui::GetIO();

    int display_w = 0;
    int display_h = 0;

    if (gHwnd)
    {
        RECT rect;
        if (GetClientRect(gHwnd, &rect))
        {
            display_w = rect.right - rect.left;
            display_h = rect.bottom - rect.top;
        }
    }

    if (display_w <= 0 || display_h <= 0) return;

    const glm::mat4 view = glm::lookAt(
        state.camera.Position,
        state.camera.Position + state.camera.Front,
        state.camera.Up
    );

    const glm::mat4 proj = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(display_w) / static_cast<float>(display_h),
        0.1f,
        100000.0f
    );

    const float mouseX = io.MousePos.x;
    const float mouseY = static_cast<float>(display_h) - io.MousePos.y;

    const glm::vec4 viewport(0.0f, 0.0f, static_cast<float>(display_w), static_cast<float>(display_h));

    const glm::vec3 rayStart = glm::unProject(
        glm::vec3(mouseX, mouseY, 0.0f),
        view,
        proj,
        viewport
    );

    const glm::vec3 rayEnd = glm::unProject(
        glm::vec3(mouseX, mouseY, 1.0f),
        view,
        proj,
        viewport
    );

    const glm::vec3 rayDir = glm::normalize(rayEnd - rayStart);

    float closestDist = std::numeric_limits<float>::max();
    uint32_t hitPropID = 0;
    int hitPropAreaIdx = -1;
    int hitSkyModelIdx = -1;

    auto& skyMgr = Sky::Manager::Instance();

    if (!skyMgr.isLoading())
    {
        size_t skyCount = skyMgr.getSkyboxM3Count();

        if (skyCount > 0)
        {
            glm::mat4 skyModel = glm::translate(glm::mat4(1.0f), state.camera.Position);

            for (size_t i = 0; i < skyCount; i++)
            {
                M3Render* m3 = skyMgr.getSkyboxM3(i);
                if (!m3 || m3->getSubmeshCount() == 0) continue;

                float dist = 0.0f;
                int hitSubmesh = m3->rayPick(rayStart, rayDir, skyModel, dist);

                if (hitSubmesh >= 0 && dist < closestDist)
                {
                    closestDist = dist;
                    hitSkyModelIdx = static_cast<int>(i);
                    hitPropID = 0;
                }
            }
        }
    }

    if (gShowProps)
    {
        for (size_t areaIdx = 0; areaIdx < gLoadedAreas.size(); ++areaIdx)
        {
            const auto& area = gLoadedAreas[areaIdx];
            if (!area) continue;

            const auto dxWorldOffset = area->getWorldOffset();
            const glm::vec3 worldOffset(dxWorldOffset.x, dxWorldOffset.y, dxWorldOffset.z);

            const auto& props = area->getProps();
            for (size_t i = 0; i < props.size(); ++i)
            {
                const Prop& prop = props[i];
                if (!prop.loaded || !prop.render) continue;
                if (IsPropHidden(prop.uniqueID) || IsPropDeleted(prop.uniqueID)) continue;

                glm::vec3 propPos = prop.position + worldOffset;

                glm::mat4 modelMatrix = glm::mat4(1.0f);
                modelMatrix = glm::translate(modelMatrix, propPos);
                modelMatrix = modelMatrix * glm::mat4_cast(prop.rotation);
                modelMatrix = glm::scale(modelMatrix, glm::vec3(prop.scale));

                float dist = 0.0f;
                int hitSubmesh = prop.render->rayPick(rayStart, rayDir, modelMatrix, dist);

                if (hitSubmesh >= 0 && dist < closestDist)
                {
                    closestDist = dist;
                    hitPropID = prop.uniqueID;
                    hitPropAreaIdx = static_cast<int>(areaIdx);
                    hitSkyModelIdx = -1;
                }
            }
        }
    }

    if (hitSkyModelIdx >= 0)
    {
        SelectSkyModel(hitSkyModelIdx);
        return;
    }

    if (hitPropID != 0)
    {
        gSelectedPropID = hitPropID;
        gSelectedPropAreaIndex = hitPropAreaIdx;
        gSelectedAreaIndex = hitPropAreaIdx;
        gSelectedSkyModelIndex = -1;
        return;
    }

    gSelectedPropID = 0;
    gSelectedPropAreaIndex = -1;
    gSelectedSkyModelIndex = -1;

    int hitAreaIdx = -1;
    std::string hitAreaName;
    closestDist = std::numeric_limits<float>::max();

    for (size_t areaIdx = 0; areaIdx < gLoadedAreas.size(); ++areaIdx)
    {
        const auto& area = gLoadedAreas[areaIdx];
        if (!area) continue;

        const auto dxWorldOffset = area->getWorldOffset();
        const auto dxLocalMin = area->getMinBounds();
        const auto dxLocalMax = area->getMaxBounds();

        const glm::vec3 worldOffset(dxWorldOffset.x, dxWorldOffset.y, dxWorldOffset.z);
        const glm::vec3 localMin(dxLocalMin.x, dxLocalMin.y, dxLocalMin.z);
        const glm::vec3 localMax(dxLocalMax.x, dxLocalMax.y, dxLocalMax.z);

        if (localMin.x > localMax.x || localMin.y > localMax.y || localMin.z > localMax.z)
            continue;

        const glm::vec3 areaWorldMin = localMin + worldOffset;
        const glm::vec3 areaWorldMax = localMax + worldOffset;

        float dist = 0.0f;
        if (RayIntersectsAABB(rayStart, rayDir, areaWorldMin, areaWorldMax, dist))
        {
            if (dist < closestDist)
            {
                closestDist = dist;
                hitAreaIdx = static_cast<int>(areaIdx);
                hitAreaName = "Area " + std::to_string(areaIdx);
            }
        }
    }

    if (hitAreaIdx >= 0)
    {
        gSelectedAreaIndex = hitAreaIdx;
        gSelectedAreaName = hitAreaName;
    }
}

void ClearPropSelection()
{
    gSelectedPropID = 0;
    gSelectedPropAreaIndex = -1;
    gSelectedAreaIndex = -1;
}

bool IsPropSelected(uint32_t uniqueID)
{
    return gSelectedPropID != 0 && gSelectedPropID == uniqueID;
}

void SelectProp(uint32_t uniqueID, int areaIndex)
{
    gSelectedPropID = uniqueID;
    gSelectedPropAreaIndex = areaIndex;
    gSelectedAreaIndex = areaIndex;
    gSelectedSkyModelIndex = -1;
}

bool IsPropVisible(uint32_t uniqueID)
{
    return !IsPropHidden(uniqueID) && !IsPropDeleted(uniqueID);
}

const Prop* GetSelectedProp()
{
    if (gSelectedPropID == 0) return nullptr;
    if (gSelectedPropAreaIndex < 0 || gSelectedPropAreaIndex >= static_cast<int>(gLoadedAreas.size()))
        return nullptr;

    const auto& area = gLoadedAreas[gSelectedPropAreaIndex];
    if (!area) return nullptr;

    return area->getPropByID(gSelectedPropID);
}

bool IsPropHidden(uint32_t uniqueID)
{
    return gHiddenProps.count(uniqueID) > 0;
}

bool IsPropDeleted(uint32_t uniqueID)
{
    return gDeletedProps.count(uniqueID) > 0;
}

void HideSelectedProp()
{
    if (gSelectedPropID != 0)
    {
        gHiddenProps.insert(gSelectedPropID);
        ClearPropSelection();
    }
}

void ShowAllHiddenProps()
{
    gHiddenProps.clear();
}

void DeleteSelectedProp()
{
    if (gSelectedPropID != 0)
    {
        gDeletedProps.insert(gSelectedPropID);
        ClearPropSelection();
    }
}

void SelectSkyModel(int index)
{
    gSelectedSkyModelIndex = index;
    gSelectedPropID = 0;
    gSelectedPropAreaIndex = -1;
    Sky::Manager::Instance().setSelectedSkyModelIndex(index);
}

void ClearSkyModelSelection()
{
    gSelectedSkyModelIndex = -1;
    Sky::Manager::Instance().setSelectedSkyModelIndex(-1);
}

bool IsSkyModelSelected(int index)
{
    return gSelectedSkyModelIndex >= 0 && gSelectedSkyModelIndex == index;
}

void HandleGlobalKeys(AppState& state)
{
    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        if (gSelectedSkyModelIndex >= 0)
        {
            ClearSkyModelSelection();
        }
        else if (gSelectedPropID != 0)
        {
            ClearPropSelection();
        }
        else if (gSelectedAreaIndex >= 0)
        {
            gSelectedAreaIndex = -1;
        }
        else if (state.m3Render)
        {
            state.m3Render->setSelectedBone(-1);
            state.m3Render->setSelectedSubmesh(-1);
        }
    }

    if (ImGui::GetIO().WantCaptureKeyboard)
        return;

    if (ImGui::IsKeyPressed(ImGuiKey_H) && !ImGui::GetIO().KeyCtrl)
    {
        HideSelectedProp();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_H) && ImGui::GetIO().KeyCtrl)
    {
        ShowAllHiddenProps();
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Delete))
    {
        DeleteSelectedProp();
    }
}