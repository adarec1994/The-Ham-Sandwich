#version 420 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aTangent;
layout (location = 3) in vec2 aUV;

out VS_OUT 
{
    vec3 FragPos;
    vec3 Normal;
    vec2 UV;
    vec4 EyeSpacePosition;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    mat4 mvMatrix = view * model;
    
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.Normal = aNormal;
    vs_out.UV = aUV;
    vs_out.EyeSpacePosition = mvMatrix * vec4(aPos, 1.0);
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}