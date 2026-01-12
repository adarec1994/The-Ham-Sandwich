#version 330 core
out vec4 FragColor;

in VS_OUT 
{
    vec3 FragPos;
} fs_in;

uniform vec3 objectColor;
uniform vec3 viewPos;

void main()
{
    // get diffuse color
    vec3 color = objectColor.rgb;
    FragColor = vec4(color, 0.5);
}