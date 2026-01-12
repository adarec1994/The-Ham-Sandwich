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
    vec3 Normal;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 viewPos;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    float nX = (aNormal.x - 127.0) / 127.0;
    float nY = (aNormal.y - 127.0) / 127.0;
    float nZ = sqrt((nX * nX) + (nY * nY));
    vs_out.Normal = vec3(nX, nY, 1.0 - nZ);

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}