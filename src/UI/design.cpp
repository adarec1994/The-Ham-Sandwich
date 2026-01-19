#include "UI.h"
#include "../Area/AreaRender.h"
#include "../resource.h"

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
#include <fstream>

bool   gAreaIconLoaded = false;
unsigned int gAreaIconTexture = 0;
int    gAreaIconWidth = 0;
int    gAreaIconHeight = 0;

#ifdef _WIN32
static bool GetResourceData(int resourceId, const void** outData, DWORD* outSize)
{
    HRSRC hRes = FindResource(nullptr, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (!hRes) return false;
    HGLOBAL hMem = LoadResource(nullptr, hRes);
    if (!hMem) return false;
    *outSize = SizeofResource(nullptr, hRes);
    *outData = LockResource(hMem);
    return *outData != nullptr;
}
#endif

static GLuint CompileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success = 0;
    char infoLog[512]{};

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
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

    const GLuint vertexShader   = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    const GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    state.grid.ShaderProgram = glCreateProgram();
    glAttachShader(state.grid.ShaderProgram, vertexShader);
    glAttachShader(state.grid.ShaderProgram, fragmentShader);
    glLinkProgram(state.grid.ShaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    std::vector<float> vertices;

    constexpr int size = 20;
    for (int i = -size; i <= size; ++i)
    {
        constexpr float step = 1.0f;

        const float fi = static_cast<float>(i) * step;
        constexpr float fs = static_cast<float>(size) * step;

        vertices.push_back(fi);   vertices.push_back(0.0f); vertices.push_back(-fs);
        vertices.push_back(fi);   vertices.push_back(0.0f); vertices.push_back( fs);

        vertices.push_back(-fs);  vertices.push_back(0.0f); vertices.push_back(fi);
        vertices.push_back( fs);  vertices.push_back(0.0f); vertices.push_back(fi);
    }

    state.grid.VertexCount = static_cast<int>(vertices.size() / 3);

    glGenVertexArrays(1, &state.grid.VAO);
    glGenBuffers(1, &state.grid.VBO);

    glBindVertexArray(state.grid.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, state.grid.VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        3 * static_cast<GLsizei>(sizeof(float)),
        nullptr
    );
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, nullptr, 4);
    if (image_data == nullptr)
        return false;

    for (int i = 0; i < image_width * image_height * 4; i += 4)
    {
        unsigned char alpha = image_data[i + 3];
        if (alpha > 0)
        {
            image_data[i]     = 255 - image_data[i];
            image_data[i + 1] = 255 - image_data[i + 1];
            image_data[i + 2] = 255 - image_data[i + 2];
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

static bool LoadTextureFromMemory(const unsigned char* data, size_t dataSize, GLuint* out_texture, int* out_width, int* out_height)
{
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load_from_memory(
        data,
        static_cast<int>(dataSize),
        &image_width, &image_height, nullptr, 4);

    if (image_data == nullptr)
        return false;

    for (int i = 0; i < image_width * image_height * 4; i += 4)
    {
        unsigned char alpha = image_data[i + 3];
        if (alpha > 0)
        {
            image_data[i]     = 255 - image_data[i];
            image_data[i + 1] = 255 - image_data[i + 1];
            image_data[i + 2] = 255 - image_data[i + 2];
        }
    }

    glGenTextures(1, out_texture);
    glBindTexture(GL_TEXTURE_2D, *out_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        image_width,
        image_height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        image_data
    );
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(image_data);

    *out_width = image_width;
    *out_height = image_height;

    return true;
}

#ifdef _WIN32
static bool LoadTextureFromResource(int resourceId, GLuint* out_texture, int* out_width, int* out_height)
{
    const void* data = nullptr;
    DWORD dataSize = 0;
    if (!GetResourceData(resourceId, &data, &dataSize))
        return false;

    return LoadTextureFromMemory(
        static_cast<const unsigned char*>(data),
        static_cast<size_t>(dataSize),
        out_texture, out_width, out_height);
}
#endif

static const char* GetResourcePath(int resourceId)
{
    switch (resourceId)
    {
        case IDR_ICON_FILETREE:  return "assets/icons/FileTree.png";
        case IDR_ICON_AREA:      return "assets/icons/Area.png";
        case IDR_ICON_SETTINGS:  return "assets/icons/Settings.png";
        case IDR_ICON_ABOUT:     return "assets/icons/About.png";
        case IDR_ICON_CHARACTER: return "assets/icons/Character.png";
        default:                 return nullptr;
    }
}

static const char* GetFontPath(int resourceId)
{
    switch (resourceId)
    {
        case IDR_FONT_ROBOTO: return "assets/fonts/Roboto-Regular.ttf";
        default:              return nullptr;
    }
}

static bool LoadTextureFromResourceCrossPlat(int resourceId, GLuint* out_texture, int* out_width, int* out_height)
{
#ifdef _WIN32
    if (LoadTextureFromResource(resourceId, out_texture, out_width, out_height))
        return true;
#endif
    const char* path = GetResourcePath(resourceId);
    if (path)
        return LoadTextureFromFile(path, out_texture, out_width, out_height);
    return false;
}

static bool LoadFontFromResourceCrossPlat(int resourceId, ImGuiIO& io, float fontSize)
{
#ifdef _WIN32
    const void* fontData = nullptr;
    DWORD fontDataSize = 0;
    if (GetResourceData(resourceId, &fontData, &fontDataSize))
    {
        void* fontDataCopy = IM_ALLOC(fontDataSize);
        memcpy(fontDataCopy, fontData, fontDataSize);
        ImFont* font = io.Fonts->AddFontFromMemoryTTF(fontDataCopy, static_cast<int>(fontDataSize), fontSize);
        if (font != nullptr)
            return true;
    }
#endif
    const char* path = GetFontPath(resourceId);
    if (path)
    {
        ImFont* font = io.Fonts->AddFontFromFileTTF(path, fontSize);
        if (font != nullptr)
            return true;
    }
    return false;
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

    if (!LoadFontFromResourceCrossPlat(IDR_FONT_ROBOTO, io, 18.0f))
    {
        io.Fonts->AddFontDefault();
    }

    state.iconLoaded = LoadTextureFromResourceCrossPlat(IDR_ICON_FILETREE, &state.iconTexture, &state.iconWidth, &state.iconHeight);
    gAreaIconLoaded = LoadTextureFromResourceCrossPlat(IDR_ICON_AREA, &gAreaIconTexture, &gAreaIconWidth, &gAreaIconHeight);
    state.settingsIconLoaded = LoadTextureFromResourceCrossPlat(IDR_ICON_SETTINGS, &state.settingsIconTexture, &state.settingsIconWidth, &state.settingsIconHeight);
    state.aboutIconLoaded = LoadTextureFromResourceCrossPlat(IDR_ICON_ABOUT, &state.aboutIconTexture, &state.aboutIconWidth, &state.aboutIconHeight);

    state.areaRender = std::make_shared<AreaRender>();
    state.areaRender->init();
}