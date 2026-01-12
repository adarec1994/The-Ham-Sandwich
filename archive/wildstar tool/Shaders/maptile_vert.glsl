#version 420 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;

out VS_OUT 
{
    vec3 FragPos;
    vec2 UV;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 viewPos;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.UV = aUV;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}