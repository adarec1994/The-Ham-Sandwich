#include "UI.h"
#include <string>
#include <vector>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL) return false;

    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);
    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;
    return true;
}

GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return shader;
}

void InitGrid(AppState& state) {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 view;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * view * vec4(aPos, 1.0);
        }
    )";
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(0.4, 0.4, 0.4, 1.0);
        }
    )";

    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    state.grid.ShaderProgram = glCreateProgram();
    glAttachShader(state.grid.ShaderProgram, vertexShader);
    glAttachShader(state.grid.ShaderProgram, fragmentShader);
    glLinkProgram(state.grid.ShaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    std::vector<float> vertices;
    int size = 20;
    float step = 1.0f;

    for (int i = -size; i <= size; ++i) {
        vertices.push_back((float)i * step); vertices.push_back(0.0f); vertices.push_back((float)-size * step);
        vertices.push_back((float)i * step); vertices.push_back(0.0f); vertices.push_back((float)size * step);

        vertices.push_back((float)-size * step); vertices.push_back(0.0f); vertices.push_back((float)i * step);
        vertices.push_back((float)size * step);  vertices.push_back(0.0f); vertices.push_back((float)i * step);
    }
    state.grid.VertexCount = (int)vertices.size() / 3;

    glGenVertexArrays(1, &state.grid.VAO);
    glGenBuffers(1, &state.grid.VBO);

    glBindVertexArray(state.grid.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, state.grid.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        AppState* state = (AppState*)glfwGetWindowUserPointer(window);
        if (state)
        {
            state->camera.MovementSpeed += (float)yoffset;
            if (state->camera.MovementSpeed < 1.0f) state->camera.MovementSpeed = 1.0f;
            if (state->camera.MovementSpeed > 10.0f) state->camera.MovementSpeed = 10.0f;
        }
    }
}

void UpdateCamera(GLFWwindow* window, AppState& state) {
    ImGuiIO& io = ImGui::GetIO();
    float dt = io.DeltaTime;

    if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
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
        if (ImGui::IsKeyDown(ImGuiKey_W))
            state.camera.Position += state.camera.Front * velocity;
        if (ImGui::IsKeyDown(ImGuiKey_S))
            state.camera.Position -= state.camera.Front * velocity;
        if (ImGui::IsKeyDown(ImGuiKey_A))
            state.camera.Position -= state.camera.Right * velocity;
        if (ImGui::IsKeyDown(ImGuiKey_D))
            state.camera.Position += state.camera.Right * velocity;

    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void RenderGrid(AppState& state, int display_w, int display_h) {
    if (display_w <= 0 || display_h <= 0) return;

    glUseProgram(state.grid.ShaderProgram);

    glm::mat4 view = glm::lookAt(state.camera.Position, state.camera.Position + state.camera.Front, state.camera.Up);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)display_w / (float)display_h, 0.1f, 100.0f);

    unsigned int viewLoc = glGetUniformLocation(state.grid.ShaderProgram, "view");
    unsigned int projLoc = glGetUniformLocation(state.grid.ShaderProgram, "projection");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(state.grid.VAO);
    glDrawArrays(GL_LINES, 0, state.grid.VertexCount);
    glBindVertexArray(0);
}

void ApplyBrainwaveStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding     = ImVec2(12, 12);
    style.FramePadding      = ImVec2(8, 6);
    style.ItemSpacing       = ImVec2(8, 8);
    style.ItemInnerSpacing  = ImVec2(6, 6);
    style.IndentSpacing     = 25.0f;
    style.ScrollbarSize     = 12.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabMinSize       = 5.0f;
    style.GrabRounding      = 3.0f;
    style.WindowRounding    = 0.0f;
    style.FrameRounding     = 4.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]          = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_WindowBg]      = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_ChildBg]       = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_PopupBg]       = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_Border]        = ImVec4(0.25f, 0.25f, 0.27f, 0.50f);
    colors[ImGuiCol_FrameBg]       = ImVec4(0.20f, 0.21f, 0.22f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]= ImVec4(0.20f, 0.21f, 0.22f, 0.80f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_TitleBg]       = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_MenuBarBg]     = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_Button]        = ImVec4(0.20f, 0.21f, 0.22f, 0.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.21f, 0.22f, 0.50f);
    colors[ImGuiCol_ButtonActive]  = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
    colors[ImGuiCol_Header]        = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    colors[ImGuiCol_HeaderActive]  = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_Separator]     = ImVec4(0.25f, 0.25f, 0.27f, 0.50f);
}

void InitUI(AppState& state) {
    ApplyBrainwaveStyle();

    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->AddFontFromFileTTF("./assets/fonts/Roboto-Regular.ttf", 18.0f);
    if (font == nullptr) io.Fonts->AddFontDefault();

    state.iconLoaded = LoadTextureFromFile("./assets/icons/FileTree.png", &state.iconTexture, &state.iconWidth, &state.iconHeight);
    if (!state.iconLoaded) printf("Failed to load icon.\n");
}

void RenderUI(AppState& state) {
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    // --- Speed Indicator Overlay ---
    ImGui::SetNextWindowPos(ImVec2(viewport->Size.x - 10.0f, 10.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGuiWindowFlags overlay_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("SpeedOverlay", nullptr, overlay_flags))
    {
        ImGui::Text("Camera Speed: %.1f", state.camera.MovementSpeed);
    }
    ImGui::End();

    // --- Left Sidebar ---
    float strip_width = 60.0f;
    float button_height = 50.0f;
    float panel_width = 280.0f;

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(strip_width, viewport->Size.y));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.09f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 10.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGuiWindowFlags strip_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("##Strip", nullptr, strip_flags)) {
        bool is_active = (state.sidebar_visible && state.active_tab_index == 0);

        if (is_active) {
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(viewport->Pos.x, ImGui::GetCursorScreenPos().y + (button_height * 0.1f)),
                ImVec2(viewport->Pos.x + 3, ImGui::GetCursorScreenPos().y + (button_height * 0.9f)),
                IM_COL32(100, 149, 237, 255)
            );
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));

        if (state.iconLoaded) {
            float icon_size = 40.0f;
            float pad_x = (strip_width - icon_size) * 0.5f;
            float pad_y = (button_height - icon_size) * 0.5f;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(pad_x, pad_y));

            if (ImGui::ImageButton("##FileTab", (void*)(intptr_t)state.iconTexture, ImVec2(icon_size, icon_size), ImVec2(0,0), ImVec2(1,1), ImVec4(0,0,0,0), ImVec4(0.6f, 0.6f, 0.6f, 1.0f))) {
                if (state.active_tab_index == 0) state.sidebar_visible = !state.sidebar_visible;
                else { state.active_tab_index = 0; state.sidebar_visible = true; }
            }
            ImGui::PopStyleVar();
        } else {
            if (ImGui::Button("Files", ImVec2(strip_width, button_height))) {
                if (state.active_tab_index == 0) state.sidebar_visible = !state.sidebar_visible;
                else { state.active_tab_index = 0; state.sidebar_visible = true; }
            }
        }
        ImGui::PopStyleColor();

        if (ImGui::Button("S", ImVec2(strip_width, button_height))) {
             state.active_tab_index = 1; state.sidebar_visible = true;
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    if (state.sidebar_visible) {
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + strip_width, viewport->Pos.y));
        ImGui::SetNextWindowSize(ImVec2(panel_width, viewport->Size.y));
        ImGuiWindowFlags panel_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

        if (ImGui::Begin("##Panel", nullptr, panel_flags)) {
            ImGui::Spacing();
            if (state.active_tab_index == 0) {
                ImGui::Text("EXPLORER");
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0, 10));

                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
                char buf[64] = "";
                ImGui::SetNextItemWidth(-1);
                ImGui::InputTextWithHint("##Search", "Search files...", buf, 64);
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();

                ImGui::Dummy(ImVec2(0, 10));

                const char* files[] = { "Main.cpp", "Engine.h", "Shader.glsl", "Texture.png", "Scene.json" };
                for (int n = 0; n < 5; n++) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
                    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
                    if (ImGui::Button(files[n], ImVec2(-1, 35))) { }
                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor();
                }
            } else if (state.active_tab_index == 1) {
                ImGui::Text("SEARCH");
                ImGui::Separator();
            }
        }
        ImGui::End();
    }
}