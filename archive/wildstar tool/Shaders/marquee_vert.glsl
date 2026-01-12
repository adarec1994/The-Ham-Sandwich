#version 420 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 aPos;

out VS_OUT 
{
    vec3 FragPos;
    vec2 UV;
    vec2 screenPos;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 viewPos;

void main()
{

    mat4 modelview = view * model;

    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.UV = aPos.xz;

    // Transform the vertex position from model space to clip space
    vec4 clipPosition = projection * modelview * vec4(aPos, 1);

    // Transform the clip space position to screen space
    vs_out.screenPos = clipPosition.xy / clipPosition.w;

    // Transform the vertex position from model space to clip space
    gl_Position = clipPosition;
}