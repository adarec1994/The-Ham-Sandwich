#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "UI/UI.h"
#include "resource.h"
#include "Area/AreaFile.h"

#define GL_SILENCE_DEPRECATION
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <Windows.h>
#endif

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

bool gCharacterIconLoaded = false;
unsigned int gCharacterIconTexture = 0;

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

static bool LoadCharacterIcon()
{
    int w, h;
    unsigned char* imageData = nullptr;

#ifdef _WIN32
    const void* data = nullptr;
    DWORD dataSize = 0;
    if (GetResourceData(IDR_ICON_CHARACTER, &data, &dataSize))
    {
        imageData = stbi_load_from_memory(
            static_cast<const unsigned char*>(data),
            static_cast<int>(dataSize),
            &w, &h, nullptr, 4);
    }
#else
    imageData = stbi_load("assets/icons/Character.png", &w, &h, nullptr, 4);
#endif

    if (!imageData)
        return false;

    for (int i = 0; i < w * h * 4; i += 4)
    {
        imageData[i + 0] = 255 - imageData[i + 0];
        imageData[i + 1] = 255 - imageData[i + 1];
        imageData[i + 2] = 255 - imageData[i + 2];
    }

    glGenTextures(1, &gCharacterIconTexture);
    glBindTexture(GL_TEXTURE_2D, gCharacterIconTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(imageData);
    return true;
}

int main()
{
    if (!glfwInit()) return 1;

    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "The Ham Sandwich", nullptr, nullptr);
    if (window == nullptr) return 1;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        return -1;
    }

    gCharacterIconLoaded = LoadCharacterIcon();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    AppState appState;
    InitUI(appState);
    InitGrid(appState);

    glfwSetWindowUserPointer(window, &appState);

    ImVec4 clear_color = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        UpdateCamera(window, appState);

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        RenderAreas(appState, display_w, display_h);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        RenderUI(appState);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {}