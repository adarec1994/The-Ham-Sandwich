#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "../tex/tex.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <memory>
#include <string>

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
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint ShaderProgram = 0;
    int VertexCount = 0;
};

struct AppState {
    int active_tab_index = 0;
    bool sidebar_visible = false;
    float sidebar_current_width = 0.0f;
    float contentWidth = 280.0f;

    std::shared_ptr<Tex::PreviewState> texPreview = std::make_shared<Tex::PreviewState>();

    bool archivesLoaded = false;
    std::vector<ArchivePtr> archives;

    AreaFilePtr currentArea;
    AreaRenderPtr areaRender;

    bool showFileDialog = false;
    std::string currentDialogPath = R"(C:\Program Files (x86)\NCSOFT\WildStar)";
    std::string selectedPath;

    GLuint iconTexture = 0;
    int iconWidth = 0;
    int iconHeight = 0;
    bool iconLoaded = false;

    GLuint settingsIconTexture = 0;
    int settingsIconWidth = 0;
    int settingsIconHeight = 0;
    bool settingsIconLoaded = false;

    GLuint aboutIconTexture = 0;
    int aboutIconWidth = 0;
    int aboutIconHeight = 0;
    bool aboutIconLoaded = false;

    bool show_settings_window = false;
    bool show_about_window = false;

    M3RenderPtr m3Render = nullptr;
    bool show_models_window = false;

    Camera camera;
    Grid grid;
};

void InitUI(AppState& state);
void InitGrid(AppState& state);
void UpdateCamera(GLFWwindow* window, AppState& state);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void RenderAreas(const AppState& state, int display_w, int display_h);
void RenderUI(AppState& state);
