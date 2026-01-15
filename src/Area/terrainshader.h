#pragma once
#include <cstdint>
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace TerrainShader
{
    struct Uniforms
    {
        GLint view = -1;
        GLint projection = -1;
        GLint model = -1;

        GLint blendMap = -1;
        GLint colorMap = -1;
        GLint hasColorMap = -1;

        GLint layer0 = -1;
        GLint layer1 = -1;
        GLint layer2 = -1;
        GLint layer3 = -1;

        GLint layer0Normal = -1;
        GLint layer1Normal = -1;
        GLint layer2Normal = -1;
        GLint layer3Normal = -1;

        GLint texScale = -1;
        GLint camPosition = -1;
        GLint highlightColor = -1;
        GLint baseColor = -1;
    };

    GLuint CreateProgram();
    void GetUniforms(GLuint program, Uniforms& out);

    extern const char* VertexSource;
    extern const char* FragmentSource;
}