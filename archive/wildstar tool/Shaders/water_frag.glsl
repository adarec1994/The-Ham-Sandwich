#version 330 core
out vec4 FragColor;

in VS_OUT 
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
} fs_in;

vec3 UnpackNormal(vec4 packednormal)
{
    vec3 normal;
    normal.xy = packednormal.wy * 2 - 1;
    normal.z = sqrt(1 - normal.x * normal.x - normal.y * normal.y);
    return normal;
}

void main()
{
    FragColor = vec4(fs_in.Color.rgb, 1.0);
}