#version 420 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec4 aTangent;
layout (location = 3) in vec2 aUV;

out VS_OUT 
{
    vec3 FragPos;
    vec4 Normal;
	float v2;
    vec2 UV;
    vec4 EyeSpacePosition;
	vec4 Tangent;
	vec4 BiTangent;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    mat4 mvMatrix = view * model;
    
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vec3 normal = aNormal.xyz;
    normal.y *= 0.3;
    vs_out.Normal = vec4(normalize(normal), 1.0);
    vs_out.UV = aUV;
    vs_out.Tangent = aTangent;
    vs_out.BiTangent.xyz = cross( aNormal, aTangent.xyz ) * aTangent.w;
    vs_out.BiTangent.w = 1.0;
    vs_out.EyeSpacePosition = mvMatrix * vec4(aPos, 1.0);
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}