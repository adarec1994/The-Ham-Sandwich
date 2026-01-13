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

    // Calculate combined world bounds of all loaded areas
    glm::vec3 combinedMin(std::numeric_limits<float>::max());
    glm::vec3 combinedMax(std::numeric_limits<float>::lowest());
    bool hasValidBounds = false;

    for (const auto& area : gLoadedAreas)
    {
        if (!area) continue;

        // Use WORLD bounds which account for tile position
        glm::vec3 minBounds = area->getWorldMinBounds();
        glm::vec3 maxBounds = area->getWorldMaxBounds();

        // Validate bounds
        if (minBounds.x <= maxBounds.x && minBounds.z <= maxBounds.z)
        {
            combinedMin = glm::min(combinedMin, minBounds);
            combinedMax = glm::max(combinedMax, maxBounds);
            hasValidBounds = true;
        }
    }

    // Fallback if no valid bounds found
    if (!hasValidBounds)
    {
        // Try to at least get world offset from first area
        auto& firstArea = gLoadedAreas.front();
        if (firstArea)
        {
            glm::vec3 offset = firstArea->getWorldOffset();
            combinedMin = offset + glm::vec3(-100.0f, 0.0f, -100.0f);
            combinedMax = offset + glm::vec3(612.0f, 100.0f, 612.0f);  // 512 + 100 margin
        }
        else
        {
            combinedMin = glm::vec3(-100.0f, 0.0f, -100.0f);
            combinedMax = glm::vec3(100.0f, 100.0f, 100.0f);
        }
    }

    // Calculate center and radius
    glm::vec3 center = (combinedMin + combinedMax) * 0.5f;
    float radius = glm::length(combinedMax - combinedMin) * 0.5f;

    // Ensure minimum radius
    if (radius < 50.0f) radius = 250.0f;

    // Set camera orientation looking down at terrain from an angle
    state.camera.Yaw = -45.0f;
    state.camera.Pitch = -30.0f;

    // Calculate front vector from yaw/pitch
    glm::vec3 front;
    front.x = cos(glm::radians(state.camera.Yaw)) * cos(glm::radians(state.camera.Pitch));
    front.y = sin(glm::radians(state.camera.Pitch));
    front.z = sin(glm::radians(state.camera.Yaw)) * cos(glm::radians(state.camera.Pitch));
    state.camera.Front = glm::normalize(front);
    state.camera.Right = glm::normalize(glm::cross(state.camera.Front, state.camera.WorldUp));
    state.camera.Up    = glm::normalize(glm::cross(state.camera.Right, state.camera.Front));

    // Position camera at distance from center, looking toward it
    float distance = radius * 1.5f;
    state.camera.Position = center - (state.camera.Front * distance);

    // Adjust movement speed based on terrain size
    state.camera.MovementSpeed = std::max(50.0f, radius * 0.5f);

    // Debug output
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
        AppState* state = (AppState*)glfwGetWindowUserPointer(window);
        if (state)
        {
            state->camera.MovementSpeed += (float)yoffset * 5.0f;
            if (state->camera.MovementSpeed < 1.0f) state->camera.MovementSpeed = 1.0f;
            if (state->camera.MovementSpeed > 500.0f) state->camera.MovementSpeed = 500.0f;
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

        if (state.camera.Pitch > 89.0f) state.camera.Pitch = 89.0f;
        if (state.camera.Pitch < -89.0f) state.camera.Pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(state.camera.Yaw)) * cos(glm::radians(state.camera.Pitch));
        front.y = sin(glm::radians(state.camera.Pitch));
        front.z = sin(glm::radians(state.camera.Yaw)) * cos(glm::radians(state.camera.Pitch));
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