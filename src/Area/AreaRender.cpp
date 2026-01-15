#include "AreaRender.h"
#include "AreaFile.h"
#include <iostream>
#include <vector>
#include <string>

AreaRender::AreaRender() = default;

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
layout (location = 2) in vec4 tangent0;
layout (location = 3) in vec2 texcoord0;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

out vec3 fragPos;
out vec3 normal;

void main(void) {
    vec4 worldPos = model * vec4(position0, 1.0);
    fragPos = worldPos.xyz;

    mat3 normalMatrix = mat3(model);
    normal = normalize(normalMatrix * normal0);

    gl_Position = projection * view * worldPos;
}
)GLSL";

    static const char* kTerrainFragmentGLSL = R"GLSL(
#version 330 core

in vec3 fragPos;
in vec3 normal;

uniform vec4 baseColor;

out vec4 FragColor;

void main() {
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    vec3 N = normalize(normal);
    float NdotL = max(dot(N, lightDir), 0.0);

    vec3 ambient = vec3(0.3);
    vec3 diffuse = vec3(0.7) * NdotL;
    vec3 lighting = ambient + diffuse;

    FragColor = vec4(lighting, 1.0) * baseColor;
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
            log.resize(len > 1 ? static_cast<size_t>(len) : 1);
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
        log.resize(len > 1 ? static_cast<size_t>(len) : 1);
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
    if (modelLoc != -1) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, identity);

    GLint colorLoc = glGetUniformLocation(mShaderProgram, "baseColor");
    if (colorLoc != -1) glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);

    AreaChunkRender::geometryInit(mShaderProgram);

    glUseProgram(0);
}