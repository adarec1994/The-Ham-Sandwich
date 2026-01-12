#version 460

layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec2 in_uv;

out vec2 vUV;

uniform mat4 model;
uniform mat4 projection;
//uniform mat4 camView;
//uniform mat4 camProjection;
//uniform vec3 position;

void main()
{
    vUV = in_uv.xy;
    gl_Position = (projection * model * vec4(in_pos.xy, 0.0, 1.0));
    //gl_Position = projection * model * vec4(in_pos.xy, 0.0, 1.0);
    //gl_Position = projection * view * model * vec4(in_pos.xy, 0.0, 1.0);
}