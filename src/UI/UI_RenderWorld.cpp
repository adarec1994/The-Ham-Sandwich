#include "UI_RenderWorld.h"
#include "../Area/AreaRender.h"
#include "UI_Globals.h"
#include "UI_Selection.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <GLFW/glfw3.h>

#include <glad/glad.h>

static GLuint gAxisVAO = 0;
static GLuint gAxisVBO = 0;
static GLuint gAxisProgram = 0;

static GLuint gHighlightVAO = 0;
static GLuint gHighlightVBO = 0;
static GLuint gHighlightProgram = 0;

static const char* axisVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vColor;
uniform mat4 mvp;
void main() {
    gl_Position = mvp * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

static const char* axisFragmentShader = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

static const char* highlightVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 mvp;
void main() {
    gl_Position = mvp * vec4(aPos, 1.0);
}
)";

static const char* highlightFragmentShader = R"(
#version 330 core
out vec4 FragColor;
uniform vec4 color;
void main() {
    FragColor = color;
}
)";

static void InitAxisGizmo()
{
    if (gAxisVAO != 0) return;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &axisVertexShader, nullptr);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &axisFragmentShader, nullptr);
    glCompileShader(fs);

    gAxisProgram = glCreateProgram();
    glAttachShader(gAxisProgram, vs);
    glAttachShader(gAxisProgram, fs);
    glLinkProgram(gAxisProgram);

    glDeleteShader(vs);
    glDeleteShader(fs);

    float axisData[] = {
        0.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
    };

    glGenVertexArrays(1, &gAxisVAO);
    glGenBuffers(1, &gAxisVBO);

    glBindVertexArray(gAxisVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gAxisVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisData), axisData, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

static void InitHighlightShader()
{
    if (gHighlightProgram != 0) return;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &highlightVertexShader, nullptr);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &highlightFragmentShader, nullptr);
    glCompileShader(fs);

    gHighlightProgram = glCreateProgram();
    glAttachShader(gHighlightProgram, vs);
    glAttachShader(gHighlightProgram, fs);
    glLinkProgram(gHighlightProgram);

    glDeleteShader(vs);
    glDeleteShader(fs);

    glGenVertexArrays(1, &gHighlightVAO);
    glGenBuffers(1, &gHighlightVBO);
}

static void RenderAxisGizmo(const AppState& state, int display_w, int display_h)
{
    InitAxisGizmo();

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    int gizmoSize = 80;
    int margin = 10;
    int stripWidth = 70;
    glViewport(stripWidth + margin, display_h - gizmoSize - margin, gizmoSize, gizmoSize);

    glDisable(GL_DEPTH_TEST);
    glUseProgram(gAxisProgram);

    glm::mat4 rotView = glm::lookAt(
        glm::vec3(0.0f) - state.camera.Front * 2.5f,
        glm::vec3(0.0f),
        state.camera.Up
    );

    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 mvp = proj * rotView;

    GLint mvpLoc = glGetUniformLocation(gAxisProgram, "mvp");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(gAxisVAO);
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, 6);
    glBindVertexArray(0);

    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glEnable(GL_DEPTH_TEST);
}

static void RenderAreaHighlight(const AreaFilePtr& area, const glm::mat4& view, const glm::mat4& projection)
{
    if (!area) return;

    InitHighlightShader();

    glm::vec3 worldOffset = area->getWorldOffset();
    glm::vec3 minB = area->getMinBounds() + worldOffset;
    glm::vec3 maxB = area->getMaxBounds() + worldOffset;

    float vertices[] = {
        // Bottom face
        minB.x, minB.y, minB.z,
        maxB.x, minB.y, minB.z,
        maxB.x, minB.y, minB.z,
        maxB.x, minB.y, maxB.z,
        maxB.x, minB.y, maxB.z,
        minB.x, minB.y, maxB.z,
        minB.x, minB.y, maxB.z,
        minB.x, minB.y, minB.z,

        // Top face
        minB.x, maxB.y, minB.z,
        maxB.x, maxB.y, minB.z,
        maxB.x, maxB.y, minB.z,
        maxB.x, maxB.y, maxB.z,
        maxB.x, maxB.y, maxB.z,
        minB.x, maxB.y, maxB.z,
        minB.x, maxB.y, maxB.z,
        minB.x, maxB.y, minB.z,

        // Vertical edges
        minB.x, minB.y, minB.z,
        minB.x, maxB.y, minB.z,
        maxB.x, minB.y, minB.z,
        maxB.x, maxB.y, minB.z,
        maxB.x, minB.y, maxB.z,
        maxB.x, maxB.y, maxB.z,
        minB.x, minB.y, maxB.z,
        minB.x, maxB.y, maxB.z,
    };

    glBindVertexArray(gHighlightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gHighlightVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glUseProgram(gHighlightProgram);

    glm::mat4 mvp = projection * view;
    GLint mvpLoc = glGetUniformLocation(gHighlightProgram, "mvp");
    GLint colorLoc = glGetUniformLocation(gHighlightProgram, "color");

    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform4f(colorLoc, 0.0f, 1.0f, 0.0f, 1.0f);

    glLineWidth(3.0f);
    glDrawArrays(GL_LINES, 0, 24);

    glBindVertexArray(0);
}

void HandleAreaPicking(AppState& state)
{
    if (gLoadedAreas.empty()) return;
    if (ImGui::GetIO().WantCaptureMouse) return;
    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) return;

    CheckAreaSelection(state);
}

void RenderAreas(const AppState& state, int display_w, int display_h)
{
    if (display_w <= 0 || display_h <= 0) return;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);

    const glm::mat4 view = glm::lookAt(
        state.camera.Position,
        state.camera.Position + state.camera.Front,
        state.camera.Up
    );

    const glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(display_w) / static_cast<float>(display_h),
        0.1f,
        100000.0f
    );

    gViewMatrix = view;
    gProjMatrix = projection;

    if (gLoadedModel)
    {
        gLoadedModel->updateAnimation(ImGui::GetIO().DeltaTime);
        gLoadedModel->render(view, projection);
        gLoadedModel->renderSkeleton(view, projection);
    }
    else if (!gLoadedAreas.empty() && state.areaRender)
    {
        const uint32_t prog = state.areaRender->getProgram();
        if (prog != 0)
        {
            glUseProgram(prog);

            const GLint viewLoc = glGetUniformLocation(prog, "view");
            const GLint projLoc = glGetUniformLocation(prog, "projection");

            if (viewLoc != -1)
                glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            if (projLoc != -1)
                glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

            for (const auto& area : gLoadedAreas)
            {
                if (area)
                    area->render(view, projection, prog, nullptr);
            }
        }

        // Render props if enabled (no auto-loading - use Load Props button)
        if (gShowProps)
        {
            for (const auto& area : gLoadedAreas)
            {
                if (area)
                    area->renderProps(view, projection);
            }
        }

        // Render highlight for selected area
        if (gSelectedAreaIndex >= 0 && gSelectedAreaIndex < static_cast<int>(gLoadedAreas.size()))
        {
            RenderAreaHighlight(gLoadedAreas[gSelectedAreaIndex], view, projection);
        }
    }

    RenderAxisGizmo(state, display_w, display_h);
}