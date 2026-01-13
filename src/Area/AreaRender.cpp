#include "AreaRender.h"
#include "AreaFile.h"
#include <iostream>
#include <vector>
#include <string>

AreaRender::AreaRender() {}

AreaRender::~AreaRender()
{
    if (mShaderProgram) glDeleteProgram(mShaderProgram);
}

void AreaRender::init()
{
    static const char* kTerrainVertexGLSL = R"GLSL(
#version 330 core

layout (location = 0) in vec3 position0;
layout (location = 1) in vec3 normal0;
layout (location = 2) in vec2 texcoord0;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

// Pass tiled UVs and the global splat UVs
out vec2 texcoord;
out vec3 normal;
out vec2 splatUV;

void main(void) {
    normal = normal0;

    // Standard UVs for tiling textures
    texcoord = texcoord0;

    // Splat UVs usually map 0..1 across the chunk
    splatUV = texcoord0;

    gl_Position = projection * view * model * vec4(position0, 1.0);
}
)GLSL";

    static const char* kTerrainFragmentGLSL = R"GLSL(
#version 330 core

in vec2 texcoord;
in vec3 normal;
in vec2 splatUV;

uniform sampler2D textures[4];
uniform sampler2D alphaTexture; // The Splat Map
uniform sampler2D colorTexture; // Optional color map

uniform int hasColorMap;
uniform vec4 highlightColor;
uniform vec4 baseColor;

out vec4 FragColor;

void main() {
    // Read the splat blend weights
    vec4 blend = texture(alphaTexture, splatUV);

    // Normalize weights if necessary (optional, but good practice)
    float total = blend.r + blend.g + blend.b + blend.a;
    if(total > 0.001) blend /= total;
    else blend = vec4(1.0, 0.0, 0.0, 0.0); // Fallback to layer 0

    // Sample terrain layers
    // Assuming simple linear tiling. You can multiply texcoord by a uniform scale here if needed.
    vec4 col0 = texture(textures[0], texcoord * 8.0);
    vec4 col1 = texture(textures[1], texcoord * 8.0);
    vec4 col2 = texture(textures[2], texcoord * 8.0);
    vec4 col3 = texture(textures[3], texcoord * 8.0);

    // Mix layers based on splat map
    vec4 finalTex = col0 * blend.r + col1 * blend.g + col2 * blend.b + col3 * blend.a;

    // Apply color map overlay if present
    if(hasColorMap == 1) {
        vec4 overlay = texture(colorTexture, splatUV);
        finalTex *= overlay * 2.0; // Multiplying to darken/lighten
    }

    FragColor = finalTex * baseColor * highlightColor;
}
)GLSL";

    auto compileShader = [](GLenum type, const char* src) -> GLuint
    {
        GLuint sh = glCreateShader(type);
        glShaderSource(sh, 1, &src, nullptr);
        glCompileShader(sh);

        GLint ok = 0;
        glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            GLint len = 0;
            glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
            std::string log;
            log.resize(len > 1 ? (size_t)len : 1);
            glGetShaderInfoLog(sh, len, nullptr, log.data());
            std::cout << "Terrain shader compile failed:\n" << log << std::endl;
            glDeleteShader(sh);
            return 0;
        }
        return sh;
    };

    if (mShaderProgram != 0)
    {
        glDeleteProgram(mShaderProgram);
        mShaderProgram = 0;
    }

    GLuint vs = compileShader(GL_VERTEX_SHADER, kTerrainVertexGLSL);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, kTerrainFragmentGLSL);
    if (!vs || !fs)
    {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return;
    }

    mShaderProgram = glCreateProgram();
    glAttachShader(mShaderProgram, vs);
    glAttachShader(mShaderProgram, fs);
    glLinkProgram(mShaderProgram);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint linkOk = 0;
    glGetProgramiv(mShaderProgram, GL_LINK_STATUS, &linkOk);
    if (!linkOk)
    {
        GLint len = 0;
        glGetProgramiv(mShaderProgram, GL_INFO_LOG_LENGTH, &len);
        std::string log;
        log.resize(len > 1 ? (size_t)len : 1);
        glGetProgramInfoLog(mShaderProgram, len, nullptr, log.data());
        std::cout << "Terrain program link failed:\n" << log << std::endl;

        glDeleteProgram(mShaderProgram);
        mShaderProgram = 0;
        return;
    }

    glUseProgram(mShaderProgram);

    GLint modelLoc = glGetUniformLocation(mShaderProgram, "model");
    float identity[16] = {
        1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
    };
    if(modelLoc != -1) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, identity);

    GLint hlLoc = glGetUniformLocation(mShaderProgram, "highlightColor");
    if(hlLoc != -1) glUniform4f(hlLoc, 1.0f, 1.0f, 1.0f, 1.0f);

    GLint colorLoc = glGetUniformLocation(mShaderProgram, "baseColor");
    if(colorLoc != -1) glUniform4f(colorLoc, 0.8f, 0.8f, 0.8f, 1.0f);

    AreaChunkRender::geometryInit(mShaderProgram);

    glUseProgram(0);
}