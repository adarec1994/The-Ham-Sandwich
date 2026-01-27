#include "UI.h"
#include "../Area/AreaFile.h"
#include <vector>
#include <cmath>
#include <limits>
#include <windows.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include "UI_ContentBrowser.h"

extern std::vector<AreaFilePtr> gLoadedAreas;
extern void CheckAreaSelection(AppState& state);

void SnapCameraToLoaded(AppState& state)
{
    if (gLoadedAreas.empty()) return;

    glm::vec3 targetMin(std::numeric_limits<float>::max());
    glm::vec3 targetMax(std::numeric_limits<float>::lowest());
    bool found = false;

    for (const auto& area : gLoadedAreas)
    {
        if (!area) continue;

        DirectX::XMFLOAT3 offset = area->getWorldOffset();

        for (const auto& chunk : area->getChunks())
        {
            if (chunk && chunk->isFullyInitialized())
            {
                DirectX::XMFLOAT3 minB = chunk->getMinBounds();
                DirectX::XMFLOAT3 maxB = chunk->getMaxBounds();
                targetMin = glm::min(targetMin, glm::vec3(minB.x, minB.y, minB.z) + glm::vec3(offset.x, offset.y, offset.z));
                targetMax = glm::max(targetMax, glm::vec3(maxB.x, maxB.y, maxB.z) + glm::vec3(offset.x, offset.y, offset.z));
                found = true;
            }
        }
    }

    if (!found)
    {
        state.camera.Position = glm::vec3(256.0f, 500.0f, 256.0f);
        state.camera.Yaw = -90.0f;
        state.camera.Pitch = -45.0f;
        state.camera.MovementSpeed = 100.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(state.camera.Yaw)) * cos(glm::radians(state.camera.Pitch));
        front.y = sin(glm::radians(state.camera.Pitch));
        front.z = sin(glm::radians(state.camera.Yaw)) * cos(glm::radians(state.camera.Pitch));
        state.camera.Front = glm::normalize(front);
        state.camera.Right = glm::normalize(glm::cross(state.camera.Front, state.camera.WorldUp));
        state.camera.Up    = glm::normalize(glm::cross(state.camera.Right, state.camera.Front));
        return;
    }

    glm::vec3 center = (targetMin + targetMax) * 0.5f;
    glm::vec3 size = targetMax - targetMin;
    float maxExtent = std::max({size.x, size.y, size.z});

    float height = std::max(200.0f, maxExtent * 0.5f);
    float distance = std::max(300.0f, maxExtent * 0.75f);

    state.camera.Position = glm::vec3(
        center.x - distance * 0.5f,
        center.y + height,
        center.z - distance * 0.5f
    );

    state.camera.Yaw = 45.0f;
    state.camera.Pitch = -30.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(state.camera.Yaw)) * cos(glm::radians(state.camera.Pitch));
    front.y = sin(glm::radians(state.camera.Pitch));
    front.z = sin(glm::radians(state.camera.Yaw)) * cos(glm::radians(state.camera.Pitch));
    state.camera.Front = glm::normalize(front);
    state.camera.Right = glm::normalize(glm::cross(state.camera.Front, state.camera.WorldUp));
    state.camera.Up    = glm::normalize(glm::cross(state.camera.Right, state.camera.Front));

    state.camera.MovementSpeed = std::max(50.0f, maxExtent * 0.25f);
}

void SnapCameraToModel(AppState& state, const glm::vec3& boundsMin, const glm::vec3& boundsMax)
{
    glm::vec3 center = (boundsMin + boundsMax) * 0.5f;
    glm::vec3 size = boundsMax - boundsMin;
    float maxExtent = std::max({size.x, size.y, size.z});

    if (maxExtent < 0.001f) maxExtent = 2.0f;

    float distance = maxExtent * 1.5f;

    state.camera.Position = glm::vec3(
        center.x,
        center.y + maxExtent * 0.3f,
        center.z + distance
    );

    state.camera.Yaw = -90.0f;
    state.camera.Pitch = -10.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(state.camera.Yaw)) * cos(glm::radians(state.camera.Pitch));
    front.y = sin(glm::radians(state.camera.Pitch));
    front.z = sin(glm::radians(state.camera.Yaw)) * cos(glm::radians(state.camera.Pitch));
    state.camera.Front = glm::normalize(front);
    state.camera.Right = glm::normalize(glm::cross(state.camera.Front, state.camera.WorldUp));
    state.camera.Up    = glm::normalize(glm::cross(state.camera.Right, state.camera.Front));

    state.camera.MovementSpeed = std::max(5.0f, maxExtent * 0.5f);
}

void SnapCameraToProp(AppState& state, const glm::vec3& position, float scale)
{
    glm::vec3 pos = position;

    float estimatedSize = std::max(2.0f, scale * 5.0f);
    float distance = estimatedSize * 3.0f;

    state.camera.Position = glm::vec3(
        pos.x - distance * 0.5f,
        pos.y + estimatedSize * 1.5f,
        pos.z - distance * 0.5f
    );

    glm::vec3 toTarget = pos - state.camera.Position;
    toTarget = glm::normalize(toTarget);

    state.camera.Yaw = glm::degrees(atan2(toTarget.z, toTarget.x));
    state.camera.Pitch = glm::degrees(asin(toTarget.y));

    if (state.camera.Pitch > 89.0f) state.camera.Pitch = 89.0f;
    if (state.camera.Pitch < -89.0f) state.camera.Pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(state.camera.Yaw)) * cos(glm::radians(state.camera.Pitch));
    front.y = sin(glm::radians(state.camera.Pitch));
    front.z = sin(glm::radians(state.camera.Yaw)) * cos(glm::radians(state.camera.Pitch));
    state.camera.Front = glm::normalize(front);
    state.camera.Right = glm::normalize(glm::cross(state.camera.Front, state.camera.WorldUp));
    state.camera.Up    = glm::normalize(glm::cross(state.camera.Right, state.camera.Front));

    state.camera.MovementSpeed = std::max(10.0f, estimatedSize * 2.0f);
}

static const float SPEED_MIN = 1.0f;
static const float SPEED_MAX = 500.0f;

void scroll_callback(HWND hwnd, short delta, AppState* state)
{
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
    {
        if (state)
        {
            float scrollDirection = (delta > 0) ? 1.0f : -1.0f;

            float currentSpeed = state->camera.MovementSpeed;
            float normalizedPos = (currentSpeed - SPEED_MIN) / (SPEED_MAX - SPEED_MIN);

            float scaleFactor = 0.05f + normalizedPos * 0.15f;

            if (scrollDirection > 0)
            {
                state->camera.MovementSpeed *= (1.0f + scaleFactor);
            }
            else
            {
                state->camera.MovementSpeed /= (1.0f + scaleFactor);
            }

            if (state->camera.MovementSpeed < SPEED_MIN) state->camera.MovementSpeed = SPEED_MIN;
            if (state->camera.MovementSpeed > SPEED_MAX) state->camera.MovementSpeed = SPEED_MAX;
        }
    }
}

static bool gCameraActive = false;
static POINT gLockPos = {0, 0};

void UpdateCamera(HWND hwnd, AppState& state)
{
    ImGuiIO& io = ImGui::GetIO();
    float dt = io.DeltaTime;

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.WantCaptureMouse)
    {
        CheckAreaSelection(state);
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !io.WantCaptureMouse)
    {
        UI_ContentBrowser::HideIfNotDocked();
    }

    bool wantCamera = ImGui::IsMouseDown(ImGuiMouseButton_Right) && !io.WantCaptureMouse;

    if (wantCamera && !gCameraActive)
    {
        gCameraActive = true;
        SetCapture(hwnd);

        GetCursorPos(&gLockPos);

        while (ShowCursor(FALSE) >= 0);
    }
    else if (!wantCamera && gCameraActive)
    {
        gCameraActive = false;
        ReleaseCapture();

        while (ShowCursor(TRUE) < 0);
    }

    if (gCameraActive)
    {
        POINT currentPos;
        GetCursorPos(&currentPos);

        float deltaX = static_cast<float>(currentPos.x - gLockPos.x);
        float deltaY = static_cast<float>(currentPos.y - gLockPos.y);

        SetCursorPos(gLockPos.x, gLockPos.y);

        state.camera.Yaw   += deltaX * state.camera.MouseSensitivity;
        state.camera.Pitch -= deltaY * state.camera.MouseSensitivity;

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
}