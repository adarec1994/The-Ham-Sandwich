#version 330 core
out vec4 FragColor;

in VS_OUT 
{
    vec3 FragPos;
    vec3 Normal;
} fs_in;

uniform vec3 objectColor;
uniform vec3 viewPos;

void main()
{
    // get diffuse color
    vec3 color = fs_in.Normal;
    FragColor = vec4(color, 1.0);
}