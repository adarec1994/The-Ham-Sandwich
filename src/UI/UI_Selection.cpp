#include "UI_Selection.h"
#include "UI_Globals.h"
#include "UI_Utils.h"

#include <limits>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <GLFW/glfw3.h>

static bool RayIntersectsBox(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 boxMin, glm::vec3 boxMax, float& t)
{
    glm::vec3 tMin = (boxMin - rayOrigin) / rayDir;
    glm::vec3 tMax = (boxMax - rayOrigin) / rayDir;
    glm::vec3 t1 = glm::min(tMin, tMax);
    glm::vec3 t2 = glm::max(tMin, tMax);
    float tNear = glm::max(glm::max(t1.x, t1.y), t1.z);
    float tFar = glm::min(glm::min(t2.x, t2.y), t2.z);
    if (tNear > tFar || tFar < 0.0f) return false;
    t = tNear;
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

    const glm::vec3 rayStart = glm::unProject(
        glm::vec3(mouseX, mouseY, 0.0f),
        view,
        proj,
        glm::vec4(0.0f, 0.0f, static_cast<float>(display_w), static_cast<float>(display_h))
    );

    const glm::vec3 rayEnd = glm::unProject(
        glm::vec3(mouseX, mouseY, 1.0f),
        view,
        proj,
        glm::vec4(0.0f, 0.0f, static_cast<float>(display_w), static_cast<float>(display_h))
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

        const glm::vec3 center = (area->getMinBounds() + area->getMaxBounds()) * 0.5f;
        const float rot = area->getRotation();

        glm::mat4 model(1.0f);
        model = glm::translate(model, center);
        model = glm::rotate(model, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(model, -center);

        const glm::mat4 invModel = glm::inverse(model);

        const glm::vec3 rayStartLocal = glm::vec3(invModel * glm::vec4(rayStart, 1.0f));
        const glm::vec3 rayDirLocal = glm::vec3(invModel * glm::vec4(rayDir, 0.0f));

        const auto& chunks = area->getChunks();
        for (size_t i = 0; i < chunks.size(); ++i)
        {
            const auto& chunk = chunks[i];
            if (!chunk) continue;

            const glm::vec3 minB = chunk->getMinBounds();
            const glm::vec3 maxB = chunk->getMaxBounds();
            if (minB.x > maxB.x) continue;

            float dist = 0.0f;
            if (RayIntersectsBox(rayStartLocal, rayDirLocal, minB, maxB, dist))
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