#include "UI_Selection.h"
#include "UI_Globals.h"
#include "UI_Utils.h"

#include <limits>
#include <string>
#include <vector>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <GLFW/glfw3.h>

static bool RayIntersectsBox(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
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

void CheckChunkSelection(AppState& state)
{
    ImGuiIO& io = ImGui::GetIO();

    int display_w = 0;
    int display_h = 0;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &display_w, &display_h);

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
    AreaChunkRenderPtr hitChunk = nullptr;
    int hitIndex = -1;
    int hitAreaIdx = -1;
    std::string hitAreaName;

    for (size_t areaIdx = 0; areaIdx < gLoadedAreas.size(); ++areaIdx)
    {
        const auto& area = gLoadedAreas[areaIdx];
        if (!area) continue;

        const glm::vec3 worldOffset = area->getWorldOffset();
        const auto& chunks = area->getChunks();

        for (size_t i = 0; i < chunks.size(); ++i)
        {
            const auto& chunk = chunks[i];
            if (!chunk) continue;

            const glm::vec3 localMin = chunk->getMinBounds();
            const glm::vec3 localMax = chunk->getMaxBounds();

            if (localMin.x > localMax.x || localMin.y > localMax.y || localMin.z > localMax.z)
                continue;

            const glm::vec3 chunkWorldMin = localMin + worldOffset;
            const glm::vec3 chunkWorldMax = localMax + worldOffset;

            float dist = 0.0f;
            if (RayIntersectsBox(rayStart, rayDir, chunkWorldMin, chunkWorldMax, dist))
            {
                if (dist < closestDist)
                {
                    closestDist = dist;
                    hitChunk = chunk;
                    hitIndex = static_cast<int>(i);
                    hitAreaIdx = static_cast<int>(areaIdx);
                    hitAreaName = "Area " + std::to_string(areaIdx);
                }
            }
        }
    }

    if (hitChunk)
    {
        gSelectedChunk = hitChunk;
        gSelectedChunkIndex = hitIndex;
        gSelectedAreaIndex = hitAreaIdx;
        gSelectedChunkAreaName = hitAreaName;
    }
}