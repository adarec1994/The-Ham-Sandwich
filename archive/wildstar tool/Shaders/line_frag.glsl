#version 330 core
out vec4 FragColor;

in VS_OUT 
{
    vec3 FragPos;
} fs_in;

uniform vec4 lineColor;
uniform vec3 viewPos;

void main()
{
    FragColor = vec4(lineColor.rgb, 1);
}