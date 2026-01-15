// src/UI/UI_RenderWorld.cpp
#include "UI_RenderWorld.h"
#include "../Area/AreaRender.h"
#include "UI_Globals.h"

#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>

void RenderAreas(const AppState& state, int display_w, int display_h)
{
    static int frameCount = 0;
    if (frameCount < 5)
    {
        std::cout << "RenderAreas called, frame " << frameCount
                  << ", areas=" << gLoadedAreas.size() << "\n";
        frameCount++;
    }

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
}
