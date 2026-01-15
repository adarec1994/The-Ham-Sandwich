#include "UI.h"
#include "../Area/AreaFile.h"
#include <vector>
#include <cmath>
#include <iostream>
#include <limits>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

extern std::vector<AreaFilePtr> gLoadedAreas;
extern void CheckChunkSelection(AppState& state);

void SnapCameraToLoaded(AppState& state)
{
    if (gLoadedAreas.empty()) return;

    glm::vec3 combinedMin(std::numeric_limits<float>::max());
    glm::vec3 combinedMax(std::numeric_limits<float>::lowest());
    bool hasValidBounds = false;

    for (const auto& area : gLoadedAreas)
    {
        if (!area) continue;

        glm::vec3 minBounds = area->getWorldMinBounds();
        glm::vec3 maxBounds = area->getWorldMaxBounds();

        if (minBounds.x <= maxBounds.x && minBounds.z <= maxBounds.z)
        {
            combinedMin = glm::min(combinedMin, minBounds);
            combinedMax = glm::max(combinedMax, maxBounds);
            hasValidBounds = true;
        }
    }

    if (!hasValidBounds)
    {
        if (auto& firstArea = gLoadedAreas.front(); firstArea)
        {
            glm::vec3 offset = firstArea->getWorldOffset();
            combinedMin = offset + glm::vec3(-100.0f, 0.0f, -100.0f);
            combinedMax = offset + glm::vec3(612.0f, 100.0f, 612.0f);
        }
        else
        {
            combinedMin = glm::vec3(-100.0f, 0.0f, -100.0f);
            combinedMax = glm::vec3(100.0f, 100.0f, 100.0f);
        }
    }

    glm::vec3 center = (combinedMin + combinedMax) * 0.5f;
    float radius = glm::length(combinedMax - combinedMin) * 0.5f;

    if (radius < 50.0f) radius = 250.0f;

    state.camera.Yaw = -45.0f;
    state.camera.Pitch = -30.0f;

    const float yawRad   = glm::radians(state.camera.Yaw);
    const float pitchRad = glm::radians(state.camera.Pitch);

    glm::vec3 front;
    front.x = glm::cos(yawRad) * glm::cos(pitchRad);
    front.y = glm::sin(pitchRad);
    front.z = glm::sin(yawRad) * glm::cos(pitchRad);

    state.camera.Front = glm::normalize(front);
    state.camera.Right = glm::normalize(glm::cross(state.camera.Front, state.camera.WorldUp));
    state.camera.Up    = glm::normalize(glm::cross(state.camera.Right, state.camera.Front));

    float distance = radius * 1.5f;
    state.camera.Position = center - (state.camera.Front * distance);

    state.camera.MovementSpeed = std::max(5.0f, radius * 0.15f);

    std::cout << "Camera snapped to terrain:\n";
    std::cout << "  World Min: (" << combinedMin.x << ", " << combinedMin.y << ", " << combinedMin.z << ")\n";
    std::cout << "  World Max: (" << combinedMax.x << ", " << combinedMax.y << ", " << combinedMax.z << ")\n";
    std::cout << "  Center: (" << center.x << ", " << center.y << ", " << center.z << ")\n";
    std::cout << "  Camera Pos: (" << state.camera.Position.x << ", " << state.camera.Position.y << ", " << state.camera.Position.z << ")\n";
    std::cout << "  Radius: " << radius << ", Distance: " << distance << "\n";
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        if (auto* state = static_cast<AppState*>(glfwGetWindowUserPointer(window)))
        {
            state->camera.MovementSpeed += static_cast<float>(yoffset) * 5.0f;

            if (state->camera.MovementSpeed < 1.0f)
                state->camera.MovementSpeed = 1.0f;
            if (state->camera.MovementSpeed > 500.0f)
                state->camera.MovementSpeed = 500.0f;
        }
    }
}

void UpdateCamera(GLFWwindow* window, AppState& state)
{
    ImGuiIO& io = ImGui::GetIO();
    float dt = io.DeltaTime;

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.WantCaptureMouse)
    {
        CheckChunkSelection(state);
    }

    if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        ImVec2 mouse_delta = io.MouseDelta;

        state.camera.Yaw   += mouse_delta.x * state.camera.MouseSensitivity;
        state.camera.Pitch -= mouse_delta.y * state.camera.MouseSensitivity;

        if (state.camera.Pitch > 89.0f)  state.camera.Pitch = 89.0f;
        if (state.camera.Pitch < -89.0f) state.camera.Pitch = -89.0f;

        const float yawRad   = glm::radians(state.camera.Yaw);
        const float pitchRad = glm::radians(state.camera.Pitch);

        glm::vec3 front;
        front.x = glm::cos(yawRad) * glm::cos(pitchRad);
        front.y = glm::sin(pitchRad);
        front.z = glm::sin(yawRad) * glm::cos(pitchRad);
        state.camera.Front = glm::normalize(front);

        state.camera.Right = glm::normalize(glm::cross(state.camera.Front, state.camera.WorldUp));
        state.camera.Up    = glm::normalize(glm::cross(state.camera.Right, state.camera.Front));

        float velocity = state.camera.MovementSpeed * dt;
        if (ImGui::IsKeyDown(ImGuiKey_W)) state.camera.Position += state.camera.Front * velocity;
        if (ImGui::IsKeyDown(ImGuiKey_S)) state.camera.Position -= state.camera.Front * velocity;
        if (ImGui::IsKeyDown(ImGuiKey_A)) state.camera.Position -= state.camera.Right * velocity;
        if (ImGui::IsKeyDown(ImGuiKey_D)) state.camera.Position += state.camera.Right * velocity;
        if (ImGui::IsKeyDown(ImGuiKey_Q)) state.camera.Position -= state.camera.Up * velocity;
        if (ImGui::IsKeyDown(ImGuiKey_E)) state.camera.Position += state.camera.Up * velocity;
    }
    else
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}