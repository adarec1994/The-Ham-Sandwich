#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTangent;
layout (location = 2) in vec2 aNormal;
layout (location = 3) in vec2 aBiTangent;
layout (location = 4) in ivec4 aBoneIndices;
layout (location = 5) in vec4 aBoneWeights;
layout (location = 6) in vec4 color0;
layout (location = 7) in vec4 color1;
layout (location = 8) in vec2 uv0;
layout (location = 9) in vec2 uv1;
layout (location = 10) in int unknown;

out VS_OUT 
{
    vec3 FragPos;
    vec4 Color0;
    vec4 Color1;
    vec2 UV0;
    vec2 UV1;
    vec3 Normal;
    vec3 Tangent;
    vec3 BiTangent;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    float nX = (aNormal.x - 127.0) / 127.0;
    float nY = (aNormal.y - 127.0) / 127.0;
    float nZ = sqrt((nX * nX) + (nY * nY));
    vec3 unpackedNormal = vec3(nX, nY, 1.0 - nZ);
    vs_out.Normal = unpackedNormal;

    float tX = (aTangent.x - 127.0) / 127.0;
    float tY = (aTangent.y - 127.0) / 127.0;
    float tZ = sqrt((tX * tX) + (tY * tY));
    vec3 unpackedTangent = vec3(tX, tY, 1.0 - tZ);
    vs_out.Tangent = unpackedTangent;

    float btX = (aBiTangent.x - 127.0) / 127.0;
    float btY = (aBiTangent.y - 127.0) / 127.0;
    float btZ = sqrt((btX * btX) + (btY * btY));
    vec3 unpackedBiTangent = vec3(btX, btY, 1.0 - btZ);
    vs_out.BiTangent = unpackedBiTangent;

    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));

    vs_out.Color0 = color0;
    vs_out.Color1 = color1;
    vs_out.UV0 = uv0;
    vs_out.UV1 = uv1;

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}