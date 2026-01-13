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
layout (location = 2) in vec4 tangent0;
layout (location = 3) in vec2 texcoord0;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

out vec3 fragPos;
out vec3 normal;
out vec4 tangent;
out vec2 texcoord;
out vec2 splatUV;

void main(void) {
    vec4 worldPos = model * vec4(position0, 1.0);
    fragPos = worldPos.xyz;

    // Transform normal to world space
    mat3 normalMatrix = mat3(model);
    normal = normalize(normalMatrix * normal0);
    tangent = vec4(normalize(normalMatrix * tangent0.xyz), tangent0.w);

    // Texture coordinates for tiling (used with per-layer scale)
    texcoord = texcoord0;

    // Splat UV: maps 0-1 range for the 65x65 blend texture
    // Since texcoord goes from 0 to 1 for the 16x16 inner grid, this works directly
    splatUV = clamp(texcoord, 0.0, 1.0);

    gl_Position = projection * view * worldPos;
}
)GLSL";

    static const char* kTerrainFragmentGLSL = R"GLSL(
#version 330 core

in vec3 fragPos;
in vec3 normal;
in vec4 tangent;
in vec2 texcoord;
in vec2 splatUV;

uniform sampler2D textures[4];
uniform sampler2D normalTextures[4];
uniform sampler2D alphaTexture;  // Splat/blend map
uniform sampler2D colorTexture;  // Optional color map

uniform int hasColorMap;
uniform vec4 texScale;  // Per-layer texture scale
uniform vec4 highlightColor;
uniform vec4 baseColor;
uniform vec3 camPosition;

out vec4 FragColor;

void main() {
    // Sample blend weights from splat map
    vec4 blend = texture(alphaTexture, splatUV);

    // Normalize blend weights
    float total = blend.r + blend.g + blend.b + blend.a;
    if (total > 0.001) {
        blend /= total;
    } else {
        blend = vec4(1.0, 0.0, 0.0, 0.0);  // Fallback to first layer
    }

    // Sample each terrain layer with its specific texture scale
    // The texScale contains metersPerTexture-derived scaling for each layer
    vec4 col0 = texture(textures[0], texcoord * texScale.x);
    vec4 col1 = texture(textures[1], texcoord * texScale.y);
    vec4 col2 = texture(textures[2], texcoord * texScale.z);
    vec4 col3 = texture(textures[3], texcoord * texScale.w);

    // Blend layers according to splat map
    vec4 albedo = col0 * blend.r + col1 * blend.g + col2 * blend.b + col3 * blend.a;

    // Apply color map if present (RGB565 baked lighting/tint)
    if (hasColorMap == 1) {
        vec4 colorTint = texture(colorTexture, splatUV);
        // Multiply blend with brightness adjustment
        albedo.rgb *= colorTint.rgb * 2.0;
    }

    // Simple lighting
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    vec3 N = normalize(normal);
    float NdotL = max(dot(N, lightDir), 0.0);

    // Ambient + diffuse lighting
    vec3 ambient = vec3(0.3);
    vec3 diffuse = vec3(0.7) * NdotL;
    vec3 lighting = ambient + diffuse;

    vec4 finalColor = vec4(albedo.rgb * lighting, albedo.a);

    // Apply highlight and base color modifiers
    finalColor *= baseColor * highlightColor;

    FragColor = finalColor;
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

    // Initialize default uniform values
    GLint modelLoc = glGetUniformLocation(mShaderProgram, "model");
    float identity[16] = {
        1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
    };
    if (modelLoc != -1) glUniformMatrix4fv(modelLoc, 1, GL_FALSE, identity);

    GLint hlLoc = glGetUniformLocation(mShaderProgram, "highlightColor");
    if (hlLoc != -1) glUniform4f(hlLoc, 1.0f, 1.0f, 1.0f, 1.0f);

    GLint colorLoc = glGetUniformLocation(mShaderProgram, "baseColor");
    if (colorLoc != -1) glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);

    GLint texScaleLoc = glGetUniformLocation(mShaderProgram, "texScale");
    if (texScaleLoc != -1) glUniform4f(texScaleLoc, 8.0f, 8.0f, 8.0f, 8.0f);

    AreaChunkRender::geometryInit(mShaderProgram);

    glUseProgram(0);

    std::cout << "Terrain shader initialized successfully.\n";
}