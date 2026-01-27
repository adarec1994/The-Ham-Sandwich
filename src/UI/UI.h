#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include "imgui.h"
#include "../tex/tex.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <memory>
#include <string>

using Microsoft::WRL::ComPtr;

class Archive;
class AreaFile;
class AreaRender;
class M3Render;

using ArchivePtr = std::shared_ptr<Archive>;
using AreaFilePtr = std::shared_ptr<AreaFile>;
using AreaRenderPtr = std::shared_ptr<AreaRender>;
using M3RenderPtr = std::shared_ptr<M3Render>;

struct Camera {
    glm::vec3 Position = glm::vec3(0.0f, 10.0f, 20.0f);
    glm::vec3 Front    = glm::vec3(0.0f, -0.3f, -1.0f);
    glm::vec3 Up       = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Right    = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 WorldUp  = glm::vec3(0.0f, 1.0f, 0.0f);

    float Yaw = -90.0f;
    float Pitch = -20.0f;
    float MovementSpeed = 50.0f;
    float MouseSensitivity = 0.1f;
};

struct Grid {
    ComPtr<ID3D11Buffer> VertexBuffer;
    ComPtr<ID3D11VertexShader> VertexShader;
    ComPtr<ID3D11PixelShader> PixelShader;
    ComPtr<ID3D11InputLayout> InputLayout;
    ComPtr<ID3D11Buffer> ConstantBuffer;
    int VertexCount = 0;
};

struct AppState {
    std::shared_ptr<Tex::PreviewState> texPreview = std::make_shared<Tex::PreviewState>();

    bool archivesLoaded = false;
    std::vector<ArchivePtr> archives;

    AreaFilePtr currentArea;
    AreaRenderPtr areaRender;

    bool showFileDialog = false;
    std::string currentDialogPath = R"(C:\Program Files (x86)\NCSOFT\WildStar)";
    std::string selectedPath;

    M3RenderPtr m3Render = nullptr;
    bool show_models_window = false;

    Camera camera;
    Grid grid;

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
};

void InitUI(AppState& state);
void InitGrid(AppState& state);
void UpdateCamera(HWND hwnd, AppState& state);
void scroll_callback(HWND hwnd, short delta, AppState* state);
void RenderAreas(const AppState& state, int display_w, int display_h);
void RenderUI(AppState& state);
bool ShouldQuit();