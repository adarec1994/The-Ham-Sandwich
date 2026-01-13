#include "UI.h"
#include "../About/about.h"
#include "../settings/Settings.h"
#include "../Archive.h"
#include "../Area/AreaFile.h"
#include "../Area/AreaRender.h"
#include "splashscreen.h"
#include "../tex/tex.h"
#include <limits>

#include <string>
#include <vector>
#include <iostream>
#include <cmath>
#include <filesystem>
#include <algorithm>
#include <map>
#include <unordered_map>

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <Windows.h>
#else
  #include <codecvt>
  #include <locale>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static std::vector<AreaFilePtr> gLoadedAreas;

static bool   gAreaIconLoaded = false;
static GLuint gAreaIconTexture = 0;
static int    gAreaIconWidth = 0;
static int    gAreaIconHeight = 0;

static AreaChunkRenderPtr gSelectedChunk = nullptr;
static int gSelectedChunkIndex = -1;
static int gSelectedAreaIndex = -1;
static std::string gSelectedChunkAreaName = "";

static std::string wstring_to_utf8(const std::wstring& str)
{
    if (str.empty()) return std::string();
#ifdef _WIN32
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0, NULL, NULL);
    std::string out(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.size(), out.data(), size_needed, NULL, NULL);
    return out;
#else
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.to_bytes(str);
#endif
}

static bool EndsWithNoCase(const std::string& s, const std::string& suffix)
{
    if (s.size() < suffix.size()) return false;
    const size_t off = s.size() - suffix.size();
    for (size_t i = 0; i < suffix.size(); ++i)
    {
        unsigned char a = (unsigned char)std::tolower((unsigned char)s[off + i]);
        unsigned char b = (unsigned char)std::tolower((unsigned char)suffix[i]);
        if (a != b) return false;
    }
    return true;
}

static bool ContainsNoCase(const std::string& haystack, const std::string& needle)
{
    if (needle.empty()) return true;
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](char a, char b) {
            return std::tolower((unsigned char)a) == std::tolower((unsigned char)b);
        }
    );
    return it != haystack.end();
}

static std::string ToLowerCopy(std::string s)
{
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

static std::string GetExtLower(const std::string& name)
{
    std::filesystem::path p(name);
    std::string e = p.extension().string();
    if (e.empty()) return "";
    return ToLowerCopy(e);
}

static std::string FormatFolderLabel(const std::string& s)
{
    std::string out;
    out.reserve(s.size() * 2);
    char prev = 0;
    for (size_t i = 0; i < s.size(); ++i)
    {
        char c = s[i];
        if (c == '_' || c == '-') c = ' ';
        bool isUpper = (c >= 'A' && c <= 'Z');
        bool prevLower = (prev >= 'a' && prev <= 'z');
        bool prevDigit = (prev >= '0' && prev <= '9');
        bool currDigit = (c >= '0' && c <= '9');

        if (i > 0 && c != ' ' && isUpper && (prevLower || prevDigit))
            out.push_back(' ');
        if (i > 0 && currDigit && !prevDigit && prev != ' ')
            out.push_back(' ');

        out.push_back(c);
        prev = c;
    }
    while (!out.empty() && out.front() == ' ') out.erase(out.begin());
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

static void SnapCameraToLoaded(AppState& state)
{
    if (gLoadedAreas.empty()) return;

    auto& targetArea = gLoadedAreas.front();
    if (!targetArea) return;

    glm::vec3 minBounds = targetArea->getMinBounds();
    glm::vec3 maxBounds = targetArea->getMaxBounds();

    if (minBounds.x > maxBounds.x)
    {
        minBounds = glm::vec3(-100.0f, 0.0f, -100.0f);
        maxBounds = glm::vec3(100.0f, 100.0f, 100.0f);
    }

    glm::vec3 center = (minBounds + maxBounds) * 0.5f;

    float radius = glm::length(maxBounds - minBounds) * 0.5f;
    if (radius < 1.0f) radius = 10.0f;

    state.camera.Yaw = -45.0f;
    state.camera.Pitch = -30.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(state.camera.Yaw)) * cos(glm::radians(state.camera.Pitch));
    front.y = sin(glm::radians(state.camera.Pitch));
    front.z = sin(glm::radians(state.camera.Yaw)) * cos(glm::radians(state.camera.Pitch));
    state.camera.Front = glm::normalize(front);
    state.camera.Right = glm::normalize(glm::cross(state.camera.Front, state.camera.WorldUp));
    state.camera.Up    = glm::normalize(glm::cross(state.camera.Right, state.camera.Front));

    float distance = radius * 2.5f;
    state.camera.Position = center - (state.camera.Front * distance);
}

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

static void CheckChunkSelection(AppState& state)
{
    ImGuiIO& io = ImGui::GetIO();
    int display_w, display_h;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &display_w, &display_h);

    if (display_w <= 0 || display_h <= 0) return;

    glm::mat4 view = glm::lookAt(state.camera.Position, state.camera.Position + state.camera.Front, state.camera.Up);
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)display_w / (float)display_h, 0.1f, 20000.0f);

    float mouseX = io.MousePos.x;
    float mouseY = (float)display_h - io.MousePos.y;

    glm::vec3 rayStart = glm::unProject(glm::vec3(mouseX, mouseY, 0.0f), view, proj, glm::vec4(0, 0, display_w, display_h));
    glm::vec3 rayEnd   = glm::unProject(glm::vec3(mouseX, mouseY, 1.0f), view, proj, glm::vec4(0, 0, display_w, display_h));
    glm::vec3 rayDir   = glm::normalize(rayEnd - rayStart);

    float closestDist = std::numeric_limits<float>::max();
    AreaChunkRenderPtr hitChunk = nullptr;
    int hitIndex = -1;
    int hitAreaIdx = -1;
    std::string hitAreaName = "";

    for (size_t areaIdx = 0; areaIdx < gLoadedAreas.size(); ++areaIdx)
    {
        auto area = gLoadedAreas[areaIdx];
        if (!area) continue;

        glm::vec3 center = (area->getMinBounds() + area->getMaxBounds()) * 0.5f;
        float rot = area->getRotation();

        glm::mat4 model(1.0f);
        model = glm::translate(model, center);
        model = glm::rotate(model, glm::radians(rot), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(model, -center);
        glm::mat4 invModel = glm::inverse(model);

        glm::vec3 rayStartLocal = glm::vec3(invModel * glm::vec4(rayStart, 1.0f));
        glm::vec3 rayDirLocal   = glm::vec3(invModel * glm::vec4(rayDir, 0.0f));

        const auto& chunks = area->getChunks();
        for (size_t i = 0; i < chunks.size(); ++i)
        {
            auto chunk = chunks[i];
            if (!chunk) continue;

            glm::vec3 minB = chunk->getMinBounds();
            glm::vec3 maxB = chunk->getMaxBounds();
            if (minB.x > maxB.x) continue;

            float dist = 0.0f;
            if (RayIntersectsBox(rayStartLocal, rayDirLocal, minB, maxB, dist))
            {
                if (dist < closestDist)
                {
                    closestDist = dist;
                    hitChunk = chunk;
                    hitIndex = (int)i;
                    hitAreaIdx = (int)areaIdx;
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

static void LoadAreasInFolder(AppState& state, ArchivePtr arc, IFileSystemEntryPtr folderEntry)
{
    if (!arc || !folderEntry || !folderEntry->isDirectory()) return;

    gLoadedAreas.clear();
    gSelectedChunk = nullptr;

    for (auto& child : folderEntry->getChildren())
    {
        if (!child || child->isDirectory()) continue;

        std::string childName = wstring_to_utf8(child->getEntryName());
        if (!EndsWithNoCase(childName, ".area")) continue;

        auto fileEntry = std::dynamic_pointer_cast<FileEntry>(child);
        if (!fileEntry) continue;

        std::cout << "Loading Area: " << childName << std::endl;

        auto af = std::make_shared<AreaFile>(arc, fileEntry);
        if (af->load())
        {
            std::cout << "Area Loaded Successfully." << std::endl;
            gLoadedAreas.push_back(af);
        }
        else
        {
            std::cout << "Failed to load Area." << std::endl;
        }
    }

    if (!gLoadedAreas.empty())
    {
        state.currentArea = gLoadedAreas.back();
        SnapCameraToLoaded(state);
    }
    else
    {
        state.currentArea.reset();
    }
}

static void LoadSingleArea(AppState& state, ArchivePtr arc, const std::shared_ptr<FileEntry>& fileEntry)
{
    if (!arc || !fileEntry) return;

    gLoadedAreas.clear();
    gSelectedChunk = nullptr;

    std::string name = wstring_to_utf8(fileEntry->getEntryName());
    std::cout << "Loading Area: " << name << std::endl;

    auto af = std::make_shared<AreaFile>(arc, fileEntry);
    if (af->load())
    {
        std::cout << "Area Loaded Successfully." << std::endl;
        gLoadedAreas.push_back(af);
        state.currentArea = af;
        SnapCameraToLoaded(state);
    }
    else
    {
        std::cout << "Failed to load Area." << std::endl;
        state.currentArea.reset();
    }
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

GLuint CompileShader(GLenum type, const char* source)
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

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        AppState* state = (AppState*)glfwGetWindowUserPointer(window);
        if (state)
        {
            state->camera.MovementSpeed += (float)yoffset * 5.0f;
            if (state->camera.MovementSpeed < 1.0f) state->camera.MovementSpeed = 1.0f;
            if (state->camera.MovementSpeed > 200.0f) state->camera.MovementSpeed = 200.0f;
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
    }
    else
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void RenderAreas(AppState& state, int display_w, int display_h)
{
    if (display_w <= 0 || display_h <= 0) return;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glm::mat4 view = glm::lookAt(state.camera.Position, state.camera.Position + state.camera.Front, state.camera.Up);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)display_w / (float)display_h, 0.1f, 20000.0f);

    if (state.areaRender)
    {
        uint32 prog = state.areaRender->getProgram();
        glUseProgram(prog);

        // Pass matrices to shader
        unsigned int viewLoc = glGetUniformLocation(prog, "view");
        unsigned int projLoc = glGetUniformLocation(prog, "projection");
        if (viewLoc != -1) glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        if (projLoc != -1) glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // FIX: Loop through ALL loaded areas, not just the "current" one
        for (const auto& area : gLoadedAreas)
        {
            if (area)
            {
                area->render(view, projection, prog, gSelectedChunk);
            }
        }
    }
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

static void PushSplashButtonColors()
{
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.22f, 0.24f, 0.28f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.30f, 0.36f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.32f, 0.34f, 0.42f, 1.00f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 10.0f));
}

static void PopSplashButtonColors()
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

static void RenderEntryRecursive_Impl(AppState& state,
                                     IFileSystemEntryPtr entry,
                                     IFileSystemEntryPtr parentDir,
                                     ArchivePtr currentArc,
                                     float& max_width,
                                     float depth)
{
    if (entry->isDirectory())
    {
        std::string name = wstring_to_utf8(entry->getEntryName());
        if (name.empty()) name = "/";

        float indent_px = depth * ImGui::GetStyle().IndentSpacing;
        float text_w = ImGui::CalcTextSize(name.c_str()).x;
        float current_w = indent_px + text_w + 50.0f;
        if (current_w > max_width) max_width = current_w;

        bool open = ImGui::TreeNode(name.c_str());
        if (open)
        {
            auto& children = entry->getChildren();
            std::vector<IFileSystemEntryPtr> folders;
            std::vector<IFileSystemEntryPtr> files;
            folders.reserve(children.size());
            files.reserve(children.size());

            for (auto& child : children)
            {
                if (child->isDirectory()) folders.push_back(child);
                else files.push_back(child);
            }

            for (auto& folder : folders)
            {
                RenderEntryRecursive_Impl(state, folder, entry, currentArc, max_width, depth + 1.0f);
            }

            ImGuiListClipper clipper;
            clipper.Begin((int)files.size());
            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                {
                    auto& f = files[i];
                    std::string fname = wstring_to_utf8(f->getEntryName());

                    float f_indent_px = (depth + 1.0f) * ImGui::GetStyle().IndentSpacing;
                    float f_text_w = ImGui::CalcTextSize(fname.c_str()).x;
                    float f_current_w = f_indent_px + f_text_w + 50.0f;
                    if (f_current_w > max_width) max_width = f_current_w;

                    if (ImGui::Selectable(fname.c_str()))
                    {
                        if (EndsWithNoCase(fname, ".area"))
                        {
                            auto fileEntry = std::dynamic_pointer_cast<FileEntry>(f);
                            if (fileEntry && currentArc)
                                LoadSingleArea(state, currentArc, fileEntry);
                        }
                        else if (EndsWithNoCase(fname, ".tex"))
                        {
                            auto fileEntry = std::dynamic_pointer_cast<FileEntry>(f);
                            if (fileEntry && currentArc)
                                Tex::OpenTexPreviewFromEntry(state, currentArc, fileEntry);
                        }
                    }
                }
            }
            ImGui::TreePop();
        }
    }
    else
    {
        std::string name = wstring_to_utf8(entry->getEntryName());
        float indent_px = depth * ImGui::GetStyle().IndentSpacing;
        float text_w = ImGui::CalcTextSize(name.c_str()).x;
        float current_w = indent_px + text_w + 50.0f;
        if (current_w > max_width) max_width = current_w;

        if (ImGui::Selectable(name.c_str()))
        {
            if (EndsWithNoCase(name, ".area"))
            {
                if (currentArc && parentDir && parentDir->isDirectory())
                {
                    LoadAreasInFolder(state, currentArc, parentDir);
                }
                else
                {
                    auto fileEntry = std::dynamic_pointer_cast<FileEntry>(entry);
                    if (fileEntry && currentArc)
                        LoadSingleArea(state, currentArc, fileEntry);
                }
            }
            else if (EndsWithNoCase(name, ".tex"))
            {
                auto fileEntry = std::dynamic_pointer_cast<FileEntry>(entry);
                if (fileEntry && currentArc)
                {
                    Tex::OpenTexPreviewFromEntry(state, currentArc, fileEntry);
                }
            }
        }
    }
}

void RenderEntryRecursive(AppState& state, IFileSystemEntryPtr entry, ArchivePtr currentArc, float& max_width, float depth)
{
    RenderEntryRecursive_Impl(state, entry, nullptr, currentArc, max_width, depth);
}

struct AreaFilterCache
{
    std::string queryLower;
    std::vector<const Archive*> archives;
    std::unordered_map<const IFileSystemEntry*, bool> subtreeHas;
    std::unordered_map<const IFileSystemEntry*, bool> dirHas;
};

static AreaFilterCache gAreaCache;

static bool ContainsLowerFast(const std::string& hayLower, const std::string& needleLower)
{
    if (needleLower.empty()) return true;
    return hayLower.find(needleLower) != std::string::npos;
}

static bool BuildAreaCacheForDir(const IFileSystemEntryPtr& entry, const std::string& queryLower)
{
    if (!entry || !entry->isDirectory())
        return false;

    const IFileSystemEntry* key = entry.get();
    auto it = gAreaCache.subtreeHas.find(key);
    if (it != gAreaCache.subtreeHas.end())
        return it->second;

    bool dirHasMatch = false;
    bool anyChildMatch = false;

    for (auto& child : entry->getChildren())
    {
        if (!child) continue;

        if (!child->isDirectory())
        {
            std::string n = wstring_to_utf8(child->getEntryName());
            if (!EndsWithNoCase(n, ".area")) continue;

            if (queryLower.empty())
            {
                dirHasMatch = true;
            }
            else
            {
                std::string nl = ToLowerCopy(n);
                if (ContainsLowerFast(nl, queryLower))
                    dirHasMatch = true;
            }
        }
    }

    for (auto& child : entry->getChildren())
    {
        if (!child || !child->isDirectory()) continue;
        if (BuildAreaCacheForDir(child, queryLower))
            anyChildMatch = true;
    }

    bool has = dirHasMatch || anyChildMatch;
    gAreaCache.dirHas[key] = dirHasMatch;
    gAreaCache.subtreeHas[key] = has;
    return has;
}

static bool AreArchivesSame(const std::vector<ArchivePtr>& arcs, const std::vector<const Archive*>& prev)
{
    if (arcs.size() != prev.size()) return false;
    for (size_t i = 0; i < arcs.size(); ++i)
        if (arcs[i].get() != prev[i]) return false;
    return true;
}

static void EnsureAreaCache(const AppState& state, const std::string& query)
{
    std::string ql = ToLowerCopy(query);

    bool needRebuild = false;
    if (ql != gAreaCache.queryLower) needRebuild = true;
    if (!AreArchivesSame(state.archives, gAreaCache.archives)) needRebuild = true;

    if (!needRebuild) return;

    gAreaCache.queryLower = ql;
    gAreaCache.archives.clear();
    gAreaCache.archives.reserve(state.archives.size());
    for (auto& a : state.archives) gAreaCache.archives.push_back(a.get());

    gAreaCache.subtreeHas.clear();
    gAreaCache.dirHas.clear();

    for (auto& archive : state.archives)
    {
        auto root = archive ? archive->getRoot() : nullptr;
        if (!root) continue;

        for (auto& child : root->getChildren())
        {
            if (child && child->isDirectory())
                BuildAreaCacheForDir(child, gAreaCache.queryLower);
        }
    }
}

static bool DirHasAreaFiles(const IFileSystemEntryPtr& dir, const std::string& query)
{
    if (!dir || !dir->isDirectory()) return false;
    const IFileSystemEntry* key = dir.get();
    auto it = gAreaCache.dirHas.find(key);
    if (it != gAreaCache.dirHas.end()) return it->second;

    std::string ql = ToLowerCopy(query);

    bool dirHasMatch = false;
    for (auto& child : dir->getChildren())
    {
        if (!child || child->isDirectory()) continue;
        std::string n = wstring_to_utf8(child->getEntryName());
        if (!EndsWithNoCase(n, ".area")) continue;
        if (ql.empty())
        {
            dirHasMatch = true;
            break;
        }
        std::string nl = ToLowerCopy(n);
        if (ContainsLowerFast(nl, ql))
        {
            dirHasMatch = true;
            break;
        }
    }
    gAreaCache.dirHas[key] = dirHasMatch;
    return dirHasMatch;
}

static bool HasAreaInSubtree(const IFileSystemEntryPtr& entry, const std::string& query)
{
    if (!entry || !entry->isDirectory())
        return false;

    const IFileSystemEntry* key = entry.get();
    auto it = gAreaCache.subtreeHas.find(key);
    if (it != gAreaCache.subtreeHas.end())
        return it->second;

    std::string ql = ToLowerCopy(query);
    return BuildAreaCacheForDir(entry, ql);
}

static void RenderAreaTreeFiltered(AppState& state,
                                  const IFileSystemEntryPtr& entry,
                                  const IFileSystemEntryPtr& parentDir,
                                  const ArchivePtr& arc,
                                  const std::string& query,
                                  float& max_width,
                                  float depth)
{
    if (!entry || !entry->isDirectory())
        return;

    const IFileSystemEntry* key = entry.get();
    auto itSub = gAreaCache.subtreeHas.find(key);
    if (itSub != gAreaCache.subtreeHas.end() && !itSub->second)
        return;

    if (!HasAreaInSubtree(entry, query))
        return;

    std::vector<IFileSystemEntryPtr> childDirs;
    childDirs.reserve(entry->getChildren().size());
    for (auto& child : entry->getChildren())
    {
        if (!child || !child->isDirectory()) continue;
        const IFileSystemEntry* ck = child.get();
        auto it = gAreaCache.subtreeHas.find(ck);
        if (it != gAreaCache.subtreeHas.end())
        {
            if (it->second) childDirs.push_back(child);
        }
        else
        {
            if (HasAreaInSubtree(child, query)) childDirs.push_back(child);
        }
    }

    bool showNode = DirHasAreaFiles(entry, query);

    if (!showNode)
    {
        ImGuiListClipper clipper;
        clipper.Begin((int)childDirs.size());
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                RenderAreaTreeFiltered(state, childDirs[i], entry, arc, query, max_width, depth);
        }
        return;
    }

    std::string name = wstring_to_utf8(entry->getEntryName());
    if (name.empty()) name = "/";

    float indent_px = depth * ImGui::GetStyle().IndentSpacing;
    float text_w = ImGui::CalcTextSize(name.c_str()).x;
    float current_w = indent_px + text_w + 50.0f;
    if (current_w > max_width) max_width = current_w;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    bool open = ImGui::TreeNodeEx(name.c_str(), flags);

    if (open)
    {
        ImGui::PushID((void*)entry.get());

        std::string loadLabel = std::string("Load ") + FormatFolderLabel(name);
        if (ImGui::Button(loadLabel.c_str()))
            LoadAreasInFolder(state, arc, entry);

        std::map<std::string, std::vector<IFileSystemEntryPtr>> byExt;

        std::string ql = ToLowerCopy(query);

        for (auto& child : entry->getChildren())
        {
            if (!child || child->isDirectory()) continue;
            std::string childName = wstring_to_utf8(child->getEntryName());

            if (!ql.empty())
            {
                std::string nl = ToLowerCopy(childName);
                if (!ContainsLowerFast(nl, ql)) continue;
            }

            std::string ext = GetExtLower(childName);
            if (ext.empty()) ext = "<none>";
            byExt[ext].push_back(child);
        }

        std::vector<std::string> exts;
        exts.reserve(byExt.size());
        for (auto& kv : byExt) exts.push_back(kv.first);

        std::sort(exts.begin(), exts.end(), [](const std::string& a, const std::string& b) {
            if (a == ".area" && b != ".area") return true;
            if (b == ".area" && a != ".area") return false;
            return a < b;
        });

        for (auto& ext : exts)
        {
            auto& vec = byExt[ext];
            std::sort(vec.begin(), vec.end(), [](const IFileSystemEntryPtr& A, const IFileSystemEntryPtr& B) {
                std::string a = wstring_to_utf8(A->getEntryName());
                std::string b = wstring_to_utf8(B->getEntryName());
                return a < b;
            });

            ImGui::Dummy(ImVec2(0, 6));
            ImGui::Separator();
            ImGui::Text("%s", ext.c_str());
            ImGui::Separator();

            ImGuiListClipper clipper;
            clipper.Begin((int)vec.size());
            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                {
                    auto& f = vec[i];
                    std::string childName = wstring_to_utf8(f->getEntryName());

                    if (ImGui::Selectable(childName.c_str()))
                    {
                        std::string lowerExt = ToLowerCopy(ext);

                        if (lowerExt == ".area")
                        {
                            auto fileEntry = std::dynamic_pointer_cast<FileEntry>(f);
                            if (fileEntry)
                                LoadSingleArea(state, arc, fileEntry);
                        }
                        else if (lowerExt == ".tex")
                        {
                            auto fileEntry = std::dynamic_pointer_cast<FileEntry>(f);
                            if (fileEntry)
                                Tex::OpenTexPreviewFromEntry(state, arc, fileEntry);
                        }
                    }
                }
            }
        }

        ImGuiListClipper dirClipper;
        dirClipper.Begin((int)childDirs.size());
        while (dirClipper.Step())
        {
            for (int i = dirClipper.DisplayStart; i < dirClipper.DisplayEnd; i++)
                RenderAreaTreeFiltered(state, childDirs[i], entry, arc, query, max_width, depth + 1.0f);
        }

        ImGui::PopID();
        ImGui::TreePop();
    }
}

void RenderUI(AppState& state)
{
    if (!state.archivesLoaded)
    {
        PushSplashButtonColors();
        RenderSplashScreen(state);
        PopSplashButtonColors();
        return;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiIO& io = ImGui::GetIO();

    float target_panel_width = state.sidebar_visible ? state.contentWidth : 0.0f;
    if (state.sidebar_visible && target_panel_width < 280.0f) target_panel_width = 280.0f;
    if (target_panel_width > viewport->Size.x * 0.5f) target_panel_width = viewport->Size.x * 0.5f;

    float slide_speed = 1800.0f;
    float step = slide_speed * io.DeltaTime;

    if (state.sidebar_current_width < target_panel_width)
    {
        state.sidebar_current_width += step;
        if (state.sidebar_current_width > target_panel_width) state.sidebar_current_width = target_panel_width;
    }
    else if (state.sidebar_current_width > target_panel_width)
    {
        state.sidebar_current_width -= step;
        if (state.sidebar_current_width < target_panel_width) state.sidebar_current_width = target_panel_width;
    }

    ImGui::SetNextWindowPos(ImVec2(viewport->Size.x - 10.0f, 10.0f), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    ImGui::SetNextWindowBgAlpha(0.35f);
    ImGuiWindowFlags overlay_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("SpeedOverlay", nullptr, overlay_flags))
    {
        ImGui::Text("Camera Speed: %.1f", state.camera.MovementSpeed);
    }
    ImGui::End();

    if (gSelectedChunk)
    {
        ImGui::SetNextWindowPos(ImVec2(viewport->Size.x - 250.0f, 80.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(240, 0));

        if (ImGui::Begin("Chunk Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Index: %d", gSelectedChunkIndex);
            if (!gSelectedChunkAreaName.empty())
                ImGui::Text("%s", gSelectedChunkAreaName.c_str());

            ImGui::Separator();

            glm::vec3 minB = gSelectedChunk->getMinBounds();
            glm::vec3 maxB = gSelectedChunk->getMaxBounds();

            ImGui::Text("Bounds Min: (%.1f, %.1f, %.1f)", minB.x, minB.y, minB.z);
            ImGui::Text("Bounds Max: (%.1f, %.1f, %.1f)", maxB.x, maxB.y, maxB.z);

            ImGui::Separator();
            ImGui::Text("Flags: 0x%X", gSelectedChunk->getFlags());
            ImGui::Text("Avg Height: %.2f", gSelectedChunk->getAverageHeight());
            ImGui::Text("Max Height: %.2f", gSelectedChunk->getMaxHeight());

            ImGui::Separator();
            if (gSelectedAreaIndex >= 0 && gSelectedAreaIndex < (int)gLoadedAreas.size()) {
                if (ImGui::Button("Rotate Area 90")) {
                    gLoadedAreas[gSelectedAreaIndex]->rotate90();
                }
            }
        }
        ImGui::End();
    }

    float strip_width = 70.0f;
    float button_height = 50.0f;

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(strip_width, viewport->Size.y));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.09f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 10.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGuiWindowFlags strip_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("##Strip", nullptr, strip_flags))
    {
        bool is_active = (state.sidebar_visible && state.active_tab_index == 0);

        if (is_active)
        {
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(viewport->Pos.x, ImGui::GetCursorScreenPos().y + (button_height * 0.1f)),
                ImVec2(viewport->Pos.x + 3, ImGui::GetCursorScreenPos().y + (button_height * 0.9f)),
                IM_COL32(100, 149, 237, 255)
            );
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));

        if (state.iconLoaded)
        {
            float icon_size = 48.0f;
            float pad_x = (strip_width - icon_size) * 0.5f;
            float pad_y = (button_height - icon_size) * 0.5f;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(pad_x, pad_y));

            if (ImGui::ImageButton("##FileTab", (void*)(intptr_t)state.iconTexture, ImVec2(icon_size, icon_size),
                                   ImVec2(0,0), ImVec2(1,1), ImVec4(0,0,0,0), ImVec4(0.6f, 0.6f, 0.6f, 1.0f)))
            {
                if (state.active_tab_index == 0) state.sidebar_visible = !state.sidebar_visible;
                else { state.active_tab_index = 0; state.sidebar_visible = true; }
            }

            ImGui::PopStyleVar();
        }
        else
        {
            if (ImGui::Button("Files", ImVec2(strip_width, button_height)))
            {
                if (state.active_tab_index == 0) state.sidebar_visible = !state.sidebar_visible;
                else { state.active_tab_index = 0; state.sidebar_visible = true; }
            }
        }

        ImGui::PopStyleColor();

        bool is_area_active = (state.sidebar_visible && state.active_tab_index == 1);

        if (is_area_active)
        {
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(viewport->Pos.x, ImGui::GetCursorScreenPos().y + (button_height * 0.1f)),
                ImVec2(viewport->Pos.x + 3, ImGui::GetCursorScreenPos().y + (button_height * 0.9f)),
                IM_COL32(100, 149, 237, 255)
            );
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));

        if (gAreaIconLoaded)
        {
            float icon_size = 48.0f;
            float pad_x = (strip_width - icon_size) * 0.5f;
            float pad_y = (button_height - icon_size) * 0.5f;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(pad_x, pad_y));

            if (ImGui::ImageButton("##AreaTab", (void*)(intptr_t)gAreaIconTexture, ImVec2(icon_size, icon_size),
                                   ImVec2(0,0), ImVec2(1,1), ImVec4(0,0,0,0), ImVec4(0.6f, 0.6f, 0.6f, 1.0f)))
            {
                if (state.active_tab_index == 1) state.sidebar_visible = !state.sidebar_visible;
                else { state.active_tab_index = 1; state.sidebar_visible = true; }
            }

            ImGui::PopStyleVar();
        }
        else
        {
            if (ImGui::Button("Area", ImVec2(strip_width, button_height)))
            {
                if (state.active_tab_index == 1) state.sidebar_visible = !state.sidebar_visible;
                else { state.active_tab_index = 1; state.sidebar_visible = true; }
            }
        }

        ImGui::PopStyleColor();

        float bottom_margin = 10.0f;
        float settings_y = ImGui::GetWindowHeight() - button_height - bottom_margin;
        float about_y = settings_y - button_height;

        ImGui::SetCursorPosY(about_y);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        if (state.aboutIconLoaded)
        {
            float icon_size = 48.0f;
            float pad_x = (strip_width - icon_size) * 0.5f;
            float pad_y = (button_height - icon_size) * 0.5f;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(pad_x, pad_y));

            if (ImGui::ImageButton("##AboutTab", (void*)(intptr_t)state.aboutIconTexture, ImVec2(icon_size, icon_size),
                                   ImVec2(0,0), ImVec2(1,1), ImVec4(0,0,0,0), ImVec4(0.6f, 0.6f, 0.6f, 1.0f)))
            {
                state.show_about_window = !state.show_about_window;
            }

            ImGui::PopStyleVar();
        }
        else
        {
            if (ImGui::Button("About", ImVec2(strip_width, button_height)))
                state.show_about_window = !state.show_about_window;
        }
        ImGui::PopStyleColor();

        ImGui::SetCursorPosY(settings_y);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        if (state.settingsIconLoaded)
        {
            float icon_size = 48.0f;
            float pad_x = (strip_width - icon_size) * 0.5f;
            float pad_y = (button_height - icon_size) * 0.5f;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(pad_x, pad_y));

            if (ImGui::ImageButton("##SettingsTab", (void*)(intptr_t)state.settingsIconTexture, ImVec2(icon_size, icon_size),
                                   ImVec2(0,0), ImVec2(1,1), ImVec4(0,0,0,0), ImVec4(0.6f, 0.6f, 0.6f, 1.0f)))
            {
                state.show_settings_window = !state.show_settings_window;
            }

            ImGui::PopStyleVar();
        }
        else
        {
            if (ImGui::Button("Set", ImVec2(strip_width, button_height)))
                state.show_settings_window = !state.show_settings_window;
        }
        ImGui::PopStyleColor();
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    if (state.sidebar_current_width > 1.0f)
    {
        ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + strip_width, viewport->Pos.y));
        ImGui::SetNextWindowSize(ImVec2(state.sidebar_current_width, viewport->Size.y));
        ImGuiWindowFlags panel_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

        if (ImGui::Begin("##Panel", nullptr, panel_flags))
        {
            ImGui::Spacing();
            if (state.active_tab_index == 0)
            {
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

                if (ImGui::BeginChild("FileTree", ImVec2(0, 0), false))
                {
                    float calculated_width = 280.0f;

                    for (auto& archive : state.archives)
                    {
                        std::filesystem::path p(archive->getPath());
                        std::string archiveName = p.filename().string();

                        float header_w = ImGui::CalcTextSize(archiveName.c_str()).x + 30.0f;
                        if (header_w > calculated_width) calculated_width = header_w;

                        if (ImGui::TreeNode(archiveName.c_str()))
                        {
                            auto root = archive->getRoot();
                            if (root)
                            {
                                for (auto child : root->getChildren())
                                    RenderEntryRecursive_Impl(state, child, root, archive, calculated_width, 1.0f);
                            }
                            ImGui::TreePop();
                        }
                    }

                    ImGui::EndChild();
                    state.contentWidth = calculated_width;
                }
            }
            else if (state.active_tab_index == 1)
            {
                ImGui::Text("AREA");
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0, 10));

                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
                static char areaBuf[128] = "";
                ImGui::SetNextItemWidth(-1);
                ImGui::InputTextWithHint("##AreaSearch", "Search areas...", areaBuf, 128);
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();

                ImGui::Dummy(ImVec2(0, 10));

                std::string areaQuery(areaBuf);
                EnsureAreaCache(state, areaQuery);

                if (ImGui::BeginChild("AreaTree", ImVec2(0, 0), false))
                {
                    float calculated_width = 280.0f;

                    for (auto& archive : state.archives)
                    {
                        auto root = archive->getRoot();
                        if (!root) continue;

                        bool archiveHasArea = false;
                        for (auto& child : root->getChildren())
                        {
                            if (!child || !child->isDirectory()) continue;
                            const IFileSystemEntry* ck = child.get();
                            auto it = gAreaCache.subtreeHas.find(ck);
                            if (it != gAreaCache.subtreeHas.end())
                            {
                                if (it->second) { archiveHasArea = true; break; }
                            }
                            else
                            {
                                if (HasAreaInSubtree(child, areaQuery)) { archiveHasArea = true; break; }
                            }
                        }
                        if (!archiveHasArea) continue;

                        for (auto& child : root->getChildren())
                        {
                            RenderAreaTreeFiltered(state, child, root, archive, areaQuery, calculated_width, 0.0f);
                        }
                    }

                    ImGui::EndChild();
                    state.contentWidth = calculated_width;
                }
            }
        }
        ImGui::End();
    }

    RenderSettingsWindow(&state.show_settings_window);
    RenderAboutWindow(&state.show_about_window);

    if (state.texPreview)
    {
        Tex::RenderTexPreviewWindow(*state.texPreview);
    }
}