#include "UI.h"
#include "../Area/AreaRender.h"

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <Windows.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#include <vector>
#include <iostream>

bool   gAreaIconLoaded = false;
unsigned int gAreaIconTexture = 0;
int    gAreaIconWidth = 0;
int    gAreaIconHeight = 0;

static GLuint CompileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return shader;
}

void InitGrid(AppState& state)
{
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

    for (int i = -size; i <= size; ++i)
    {
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

bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL) return false;

    for (int i = 0; i < image_width * image_height * 4; i += 4)
    {
        unsigned char alpha = image_data[i + 3];
        if (alpha > 0)
        {
            image_data[i]   = 255 - image_data[i];
            image_data[i+1] = 255 - image_data[i+1];
            image_data[i+2] = 255 - image_data[i+2];
        }
    }

    glGenTextures(1, out_texture);
    glBindTexture(GL_TEXTURE_2D, *out_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(image_data);
    *out_width = image_width;
    *out_height = image_height;
    return true;
}

void ApplyBrainwaveStyle()
{
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
    colors[ImGuiCol_Text]           = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_WindowBg]       = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_ChildBg]        = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_PopupBg]        = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_Border]         = ImVec4(0.25f, 0.25f, 0.27f, 0.50f);
    colors[ImGuiCol_FrameBg]        = ImVec4(0.20f, 0.21f, 0.22f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.21f, 0.22f, 0.80f);
    colors[ImGuiCol_FrameBgActive]  = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    colors[ImGuiCol_TitleBg]        = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgActive]  = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_MenuBarBg]      = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_Button]         = ImVec4(0.20f, 0.21f, 0.22f, 0.00f);
    colors[ImGuiCol_ButtonHovered]  = ImVec4(0.20f, 0.21f, 0.22f, 0.50f);
    colors[ImGuiCol_ButtonActive]   = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
    colors[ImGuiCol_Header]         = ImVec4(0.20f, 0.21f, 0.22f, 1.00f);
    colors[ImGuiCol_HeaderHovered]  = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    colors[ImGuiCol_HeaderActive]   = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_Separator]      = ImVec4(0.25f, 0.25f, 0.27f, 0.50f);
}

void PushSplashButtonColors()
{
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.22f, 0.24f, 0.28f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.30f, 0.36f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.32f, 0.34f, 0.42f, 1.00f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 10.0f));
}

void PopSplashButtonColors()
{
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
}

void InitUI(AppState& state)
{
    ApplyBrainwaveStyle();

    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->AddFontFromFileTTF("./assets/fonts/Roboto-Regular.ttf", 18.0f);
    if (font == nullptr) io.Fonts->AddFontDefault();

    state.iconLoaded = LoadTextureFromFile("./assets/icons/FileTree.png", &state.iconTexture, &state.iconWidth, &state.iconHeight);
    if (!state.iconLoaded) printf("Failed to load FileTree icon.\n");

    gAreaIconLoaded = LoadTextureFromFile("./assets/icons/Area.png", &gAreaIconTexture, &gAreaIconWidth, &gAreaIconHeight);
    if (!gAreaIconLoaded) printf("Failed to load Area icon.\n");

    state.settingsIconLoaded = LoadTextureFromFile("./assets/icons/Settings.png", &state.settingsIconTexture, &state.settingsIconWidth, &state.settingsIconHeight);
    if (!state.settingsIconLoaded) printf("Failed to load Settings icon.\n");

    state.aboutIconLoaded = LoadTextureFromFile("./assets/icons/About.png", &state.aboutIconTexture, &state.aboutIconWidth, &state.aboutIconHeight);
    if (!state.aboutIconLoaded) printf("Failed to load About icon.\n");

    state.areaRender = std::make_shared<AreaRender>();
    state.areaRender->init();
}