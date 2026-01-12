#version 330 core
out vec4 outDiffuse;
out vec4 outSpecular;
out vec4 outNormal;
out vec4 outUnknown;

in VS_OUT 
{
    vec3 FragPos;
    vec4 Color0;
    vec4 Color1;
    vec2 UV0;
    vec2 UV1;
    vec3 Normal;
    vec3 Tangent;
    vec3 BiTangent;
} fs_in;

uniform sampler2D diffuseMap0;
uniform sampler2D normalMap0;
uniform sampler2D diffuseMap1;
uniform sampler2D normalMap1;
uniform sampler2D diffuseMap2;
uniform sampler2D normalMap2;
uniform sampler2D diffuseMap3;
uniform sampler2D normalMap3;

uniform vec4 lightColor;
uniform vec4 ambientColor;
uniform vec4 objectColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

vec3 UnpackNormal(vec4 packednormal)
{
    vec3 normal;
    normal.xy = packednormal.wy * 2 - 1;
    normal.z = sqrt(1 - normal.x * normal.x - normal.y * normal.y);
    return normal;
}

void main()
{
    vec4 color0 = texture(diffuseMap0, fs_in.UV0);
    vec4 color1 = texture(diffuseMap1, fs_in.UV0);
    vec4 color2 = texture(diffuseMap2, fs_in.UV0);
    vec4 color3 = texture(diffuseMap3, fs_in.UV0);

    vec4 normal0 = texture(normalMap0, fs_in.UV0);
    vec4 normal1 = texture(normalMap1, fs_in.UV0);
    vec4 normal2 = texture(normalMap2, fs_in.UV0);
    vec4 normal3 = texture(normalMap3, fs_in.UV0);

    vec4 colorData = (color0 * fs_in.Color1.b) + (color1 * fs_in.Color1.g) + (color2 * fs_in.Color1.r) + (color3 * fs_in.Color1.a);
    vec4 normalData = (normal0 * fs_in.Color1.b) + (normal1 * fs_in.Color1.g) + (normal2 * fs_in.Color1.r) + (normal3 * fs_in.Color1.a);

    vec3 normal = UnpackNormal(normalData);
   
    // get diffuse color
    vec3 color = colorData.rgb;

    // This must only be used where it's activated in material
    //if (colorData.a < 0.1)
    //    discard;

    outDiffuse = colorData;
    outSpecular = vec4(0,0,0,1);
    outNormal = vec4(normal.xyz, 1.0);
    outUnknown = vec4(0,0,0,0);
}