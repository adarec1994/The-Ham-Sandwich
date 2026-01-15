#include "TerrainShader.h"
#include <iostream>

namespace TerrainShader
{
    const char* VertexSource = R"(
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aTangent;
layout (location = 3) in vec2 aTexCoord;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec2 BlendCoord;
out mat3 TBN;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = normalize(normalMatrix * aNormal);

    vec3 T = normalize(normalMatrix * aTangent.xyz);
    vec3 N = Normal;
    vec3 B = cross(N, T) * aTangent.w;
    TBN = mat3(T, B, N);

    TexCoord = aTexCoord;
    BlendCoord = aTexCoord;

    gl_Position = projection * view * worldPos;
}
)";

    const char* FragmentSource = R"(
#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec2 BlendCoord;
in mat3 TBN;

uniform sampler2D blendMap;
uniform sampler2D colorMap;
uniform int hasColorMap;

uniform sampler2D layer0;
uniform sampler2D layer1;
uniform sampler2D layer2;
uniform sampler2D layer3;

uniform sampler2D layer0Normal;
uniform sampler2D layer1Normal;
uniform sampler2D layer2Normal;
uniform sampler2D layer3Normal;

uniform vec4 texScale;
uniform vec3 camPosition;
uniform vec4 highlightColor;
uniform vec4 baseColor;

out vec4 FragColor;

vec3 sampleNormal(sampler2D normalTex, vec2 uv)
{
    vec3 n = texture(normalTex, uv).rgb;
    return normalize(n * 2.0 - 1.0);
}

void main()
{
    vec4 blend = texture(blendMap, BlendCoord);

    float blendSum = blend.r + blend.g + blend.b + blend.a;
    if (blendSum > 0.001)
        blend /= blendSum;
    else
        blend = vec4(1.0, 0.0, 0.0, 0.0);

    vec2 uv0 = TexCoord * texScale.x;
    vec2 uv1 = TexCoord * texScale.y;
    vec2 uv2 = TexCoord * texScale.z;
    vec2 uv3 = TexCoord * texScale.w;

    vec4 col0 = texture(layer0, uv0);
    vec4 col1 = texture(layer1, uv1);
    vec4 col2 = texture(layer2, uv2);
    vec4 col3 = texture(layer3, uv3);

    vec4 diffuse = col0 * blend.r + col1 * blend.g + col2 * blend.b + col3 * blend.a;

    vec3 n0 = sampleNormal(layer0Normal, uv0);
    vec3 n1 = sampleNormal(layer1Normal, uv1);
    vec3 n2 = sampleNormal(layer2Normal, uv2);
    vec3 n3 = sampleNormal(layer3Normal, uv3);

    vec3 blendedNormal = normalize(n0 * blend.r + n1 * blend.g + n2 * blend.b + n3 * blend.a);
    vec3 worldNormal = normalize(TBN * blendedNormal);

    if (hasColorMap > 0)
    {
        vec4 tint = texture(colorMap, BlendCoord);
        diffuse.rgb *= tint.rgb * 2.0;
    }

    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    float NdotL = max(dot(worldNormal, lightDir), 0.0);

    vec3 ambient = vec3(0.3);
    vec3 lighting = ambient + vec3(0.7) * NdotL;

    vec3 finalColor = diffuse.rgb * lighting * baseColor.rgb;

    if (highlightColor.a > 0.0)
    {
        finalColor = mix(finalColor, highlightColor.rgb, highlightColor.a * 0.3);
    }

    FragColor = vec4(finalColor, 1.0);
}
)";

    static GLuint CompileShader(GLenum type, const char* source)
    {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char log[512];
            glGetShaderInfoLog(shader, 512, nullptr, log);
            std::cerr << "Terrain shader compile error:\n" << log << std::endl;
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    GLuint CreateProgram()
    {
        GLuint vs = CompileShader(GL_VERTEX_SHADER, VertexSource);
        if (vs == 0) return 0;

        GLuint fs = CompileShader(GL_FRAGMENT_SHADER, FragmentSource);
        if (fs == 0)
        {
            glDeleteShader(vs);
            return 0;
        }

        GLuint program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        glDeleteShader(vs);
        glDeleteShader(fs);

        GLint success;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success)
        {
            char log[512];
            glGetProgramInfoLog(program, 512, nullptr, log);
            std::cerr << "Terrain shader link error:\n" << log << std::endl;
            glDeleteProgram(program);
            return 0;
        }

        std::cout << "Terrain shader compiled successfully\n";
        return program;
    }

    void GetUniforms(GLuint program, Uniforms& out)
    {
        out.view = glGetUniformLocation(program, "view");
        out.projection = glGetUniformLocation(program, "projection");
        out.model = glGetUniformLocation(program, "model");

        out.blendMap = glGetUniformLocation(program, "blendMap");
        out.colorMap = glGetUniformLocation(program, "colorMap");
        out.hasColorMap = glGetUniformLocation(program, "hasColorMap");

        out.layer0 = glGetUniformLocation(program, "layer0");
        out.layer1 = glGetUniformLocation(program, "layer1");
        out.layer2 = glGetUniformLocation(program, "layer2");
        out.layer3 = glGetUniformLocation(program, "layer3");

        out.layer0Normal = glGetUniformLocation(program, "layer0Normal");
        out.layer1Normal = glGetUniformLocation(program, "layer1Normal");
        out.layer2Normal = glGetUniformLocation(program, "layer2Normal");
        out.layer3Normal = glGetUniformLocation(program, "layer3Normal");

        out.texScale = glGetUniformLocation(program, "texScale");
        out.camPosition = glGetUniformLocation(program, "camPosition");
        out.highlightColor = glGetUniformLocation(program, "highlightColor");
        out.baseColor = glGetUniformLocation(program, "baseColor");
    }
}