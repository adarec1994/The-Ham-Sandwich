#include "AreaRender.h"
#include <iostream>

AreaRender::AreaRender() {}

AreaRender::~AreaRender() {
    if (mShaderProgram) glDeleteProgram(mShaderProgram);
}

void AreaRender::init() {
    const char* vShaderCode = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;

        out vec3 FragPos;
        out vec3 Normal;
        out float Height;

        uniform mat4 view;
        uniform mat4 projection;

        void main() {
            FragPos = aPos;
            Normal = aNormal;
            Height = aPos.y;
            gl_Position = projection * view * vec4(aPos, 1.0);
        }
    )";

    const char* fShaderCode = R"(
        #version 330 core
        out vec4 FragColor;

        in vec3 FragPos;
        in vec3 Normal;
        in float Height;

        void main() {
            vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
            float diff = max(dot(Normal, lightDir), 0.2);
            vec3 colorLow = vec3(0.2, 0.6, 0.2);
            vec3 colorHigh = vec3(0.6, 0.5, 0.3);
            float t = clamp((Height + 2000.0) / 1000.0, 0.0, 1.0);
            vec3 color = mix(colorLow, colorHigh, t);
            FragColor = vec4(color * diff, 1.0);
        }
    )";

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);

    mShaderProgram = glCreateProgram();
    glAttachShader(mShaderProgram, vertex);
    glAttachShader(mShaderProgram, fragment);
    glLinkProgram(mShaderProgram);

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}