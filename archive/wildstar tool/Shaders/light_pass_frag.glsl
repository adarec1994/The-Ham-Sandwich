#version 420 core
out vec4 FragColor;
in vec2 TexCoords;

struct FogParameters
{
	vec3 color;
	float linearStart;
	float linearEnd;
	float density;
	
	int equation;
	bool isEnabled;
};

struct SunParameters
{
    vec3 color;
    vec3 direction;
    float intensity;

    bool isEnabled;
};

struct EnvironmentParameters
{
    vec3 ambientColor;
    FogParameters fogParams;
    SunParameters sunParams;

    bool isEnabled;
};

layout(binding=0) uniform sampler2D gDiffuse;
layout(binding=1) uniform sampler2D gSpecular;
layout(binding=2) uniform sampler2D gNormal;
layout(binding=3) uniform sampler2D gMisc;

uniform EnvironmentParameters envParams;
uniform vec3 viewPos;

void main()
{       
/*
    // retrieve data from gbuffer
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;
    
    // then calculate lighting as usual
    vec3 lighting  = Diffuse * 0.1; // hard-coded ambient component
    vec3 viewDir  = normalize(viewPos - FragPos);
    for(int i = 0; i < NR_LIGHTS; ++i)
    {
        // calculate distance between light source and current fragment
        float distance = length(lights[i].Position - FragPos);
        if(distance < lights[i].Radius)
        {
            // diffuse
            vec3 lightDir = normalize(lights[i].Position - FragPos);
            vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lights[i].Color;
            // specular
            vec3 halfwayDir = normalize(lightDir + viewDir);  
            float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
            vec3 specular = lights[i].Color * spec * Specular;
            // attenuation
            float attenuation = 1.0 / (1.0 + lights[i].Linear * distance + lights[i].Quadratic * distance * distance);
            diffuse *= attenuation;
            specular *= attenuation;
            lighting += diffuse + specular;
        }
    }    
    */
    vec4 Diffuse = texture(gDiffuse, TexCoords);
    vec4 Specular = texture(gSpecular, TexCoords);
    vec4 Normal = texture(gNormal, TexCoords);
    FragColor = vec4(Diffuse.xyz, 1.0);
}