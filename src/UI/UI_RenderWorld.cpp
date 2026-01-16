#include "UI_RenderWorld.h"
#include "../Area/AreaRender.h"
#include "UI_Globals.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>

static GLuint gAxisVAO = 0;
static GLuint gAxisVBO = 0;
static GLuint gAxisProgram = 0;

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

    if (gLoadedModel)
    {
        gLoadedModel->render(view, projection);
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
                    area->render(view, projection, prog, gSelectedChunk);
            }
        }
    }

    RenderAxisGizmo(state, display_w, display_h);
}