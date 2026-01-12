#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aTangent;
layout (location = 3) in vec4 aBiTangent;
layout (location = 4) in vec2 aUV;
layout (location = 5) in vec4 aColor;
layout (location = 6) in float aUnk0;
layout (location = 7) in int aUnk1;
layout (location = 8) in vec4 aBlendMask;

out VS_OUT 
{
    vec3 Pos;
    vec3 Normal;
    vec4 Tangent;
    vec4 BiTangent;
    vec2 UV;
    vec4 Color;
    float Unk0;
    int Unk1;
    vec4 BlendMask;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vs_out.Pos = aPos;
    vs_out.Normal = aNormal;
    vs_out.Tangent = aTangent;
    vs_out.BiTangent = aBiTangent;
    vs_out.UV = aUV;
    vs_out.Color = aColor;
    vs_out.Unk0 = aUnk0;
    vs_out.Unk1 = aUnk1;
    vs_out.BlendMask = aBlendMask;

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}