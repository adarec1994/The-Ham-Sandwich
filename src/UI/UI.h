#pragma once

#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include "imgui.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Camera {
    glm::vec3 Position = glm::vec3(0.0f, 5.0f, 10.0f);
    glm::vec3 Front    = glm::vec3(0.0f, -0.5f, -1.0f);
    glm::vec3 Up       = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 Right    = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 WorldUp  = glm::vec3(0.0f, 1.0f, 0.0f);

    float Yaw   = -90.0f;
    float Pitch = -20.0f;
    float MovementSpeed = 5.0f;
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
    bool sidebar_visible = true;

    GLuint iconTexture = 0;
    int iconWidth = 0;
    int iconHeight = 0;
    bool iconLoaded = false;

    Camera camera;
    Grid grid;
};

void InitUI(AppState& state);
void InitGrid(AppState& state);
void UpdateCamera(GLFWwindow* window, AppState& state);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void RenderGrid(AppState& state, int display_w, int display_h);
void RenderUI(AppState& state);