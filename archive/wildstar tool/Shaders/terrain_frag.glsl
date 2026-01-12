#version 420 core
out vec4 FragColor;

in VS_OUT 
{
    vec3 FragPos;
    vec3 Normal;
    vec2 UV;
    vec4 EyeSpacePosition;
} fs_in;

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

struct BrushParameters
{
    int mode;
    vec3 position;
    float size;

    bool isEnabled;
};

struct TerrainEditorParameters
{
    bool enableAreaGrid;
    bool enableChunkGrid;
    BrushParameters brushParams;
};

struct TerrainParameters
{
    vec4 heightScale;
    vec4 heightOffset;
    vec4 parallaxScale;
    vec4 parallaxOffset;
    vec4 metersPerTextureTile;// = vec4(32.0, 32.0, 32.0, 32.0);
    float specularPower;
    vec2 scrollSpeed;
    bool enableColorMap;
    bool enableUnkMap2;
};

float getFogFactor(FogParameters params, float fogCoordinate)
{
	float result = 0.0;

	if(params.equation == 0)
	{
		float fogLength = params.linearEnd - params.linearStart;
		result = (params.linearEnd - fogCoordinate) / fogLength;
	}
	else if(params.equation == 1)
    {
		result = exp(-params.density * fogCoordinate);
	}
	else if(params.equation == 2)
    {
		result = exp(-pow(params.density * fogCoordinate, 2.0));
	}
	
	result = 1.0 - clamp(result, 0.0, 1.0);
	return result;
}

layout(binding=0) uniform sampler2D layer0;
layout(binding=1) uniform sampler2D layer1;
layout(binding=2) uniform sampler2D layer2;
layout(binding=3) uniform sampler2D layer3;
layout(binding=4) uniform sampler2D normal0;
layout(binding=5) uniform sampler2D normal1;
layout(binding=6) uniform sampler2D normal2;
layout(binding=7) uniform sampler2D normal3;
layout(binding=8) uniform sampler2D blendMap;
layout(binding=9) uniform sampler2D colorMap;
layout(binding=10) uniform sampler2D unkMap2;

uniform TerrainParameters terrainParams;
uniform TerrainEditorParameters tEditorParams;
uniform EnvironmentParameters envParams;

vec3 UnpackWSNormal(vec4 packednormal)
{
    vec3 normal;
    normal.xy = packednormal.yw * 2.0 - 1.0;
    normal.z = sqrt(1.0 - normal.x * normal.x - normal.y * normal.y);
    return normal;
}

float dist(vec2 p0, vec2 pf){return sqrt((pf.x-p0.x)*(pf.x-p0.x)+(pf.y-p0.y)*(pf.y-p0.y));}

void main()
{
    vec2 uv = fs_in.UV;
    vec3 albedo = vec3(0.0, 0.0, 0.0);

    // Layer scale
    vec2 tc0 = uv * (32.0 / terrainParams.metersPerTextureTile.x);
    vec2 tc1 = uv * (32.0 / terrainParams.metersPerTextureTile.y);
    vec2 tc2 = uv * (32.0 / terrainParams.metersPerTextureTile.z);
    vec2 tc3 = uv * (32.0 / terrainParams.metersPerTextureTile.w);

    vec4 normal0Sampler = texture(normal0, tc0);
    vec4 normal1Sampler = texture(normal1, tc1);
    vec4 normal2Sampler = texture(normal2, tc2);
    vec4 normal3Sampler = texture(normal3, tc3);

    float height0 = normal0Sampler.r;
    float height1 = normal1Sampler.r;
    float height2 = normal2Sampler.r;
    float height3 = normal3Sampler.r;

    vec4 blendSampler = texture(blendMap, uv);
    vec4 colorSampler = texture(colorMap, uv);

    vec2 parallax0 = vec2(0.0, 0.0);// ParallaxOffset(height0, parallaxScale.r, IN.viewDir);
    vec2 parallax1 = vec2(0.0, 0.0);// ParallaxOffset(height1, parallaxScale.g, IN.viewDir);
    vec2 parallax2 = vec2(0.0, 0.0);// ParallaxOffset(height2, parallaxScale.b, IN.viewDir);
    vec2 parallax3 = vec2(0.0, 0.0);// ParallaxOffset(height3, parallaxScale.a, IN.viewDir);

    // Blend Normals //
    vec3 normal0 = UnpackWSNormal(normal0Sampler);
    vec3 normal1 = UnpackWSNormal(normal1Sampler);
    vec3 normal2 = UnpackWSNormal(normal2Sampler);
    vec3 normal3 = UnpackWSNormal(normal3Sampler);
    //o.Normal = clamp(normal0 * blendSampler.r, 0, 1) + clamp(normal1 * blendSampler.g, 0, 1) + clamp(normal2 * blendSampler.b, 0, 1) + normal3 * (1.0 - blendSampler.r - blendSampler.g - blendSampler.b);

    // Blend Colors //
    vec4 layer0Sampler = texture(layer0, tc0 + parallax1);
    vec4 layer1Sampler = texture(layer1, tc1 + parallax1);
    vec4 layer2Sampler = texture(layer2, tc2 + parallax2);
    vec4 layer3Sampler = texture(layer3, tc3 + parallax3);

    vec4 layer_weights = vec4(blendSampler.rgb, clamp((1.0 - blendSampler.r - blendSampler.g - blendSampler.b), 0.0, 1.0));
    vec4 layer_pct = vec4(
                        layer_weights.x * (height0 * terrainParams.heightScale[0] + terrainParams.heightOffset[0]),
                        layer_weights.y * (height1 * terrainParams.heightScale[1] + terrainParams.heightOffset[1]),
                        layer_weights.z * (height2 * terrainParams.heightScale[2] + terrainParams.heightOffset[2]),
                        layer_weights.w * (height3 * terrainParams.heightScale[3] + terrainParams.heightOffset[3])
                        );

    float max1 = max(layer_pct.x, layer_pct.y);
    float max2 = max(layer_pct.z, layer_pct.w);
    float max3 = max(max1, max2);
    vec4 layer_pct_max = vec4(max3, max3, max3, max3);
    vec4 scale = vec4(1.0, 1.0, 1.0, 1.0) - clamp(layer_pct_max - layer_pct, 0.0, 1.0);
    layer_pct = layer_pct * scale;
    float sum2 = dot(vec4(1.0, 1.0, 1.0, 1.0), layer_pct);
    layer_pct = layer_pct / vec4(sum2, sum2, sum2, sum2);

    vec4 weightedLayer_0 = layer0Sampler * layer_pct.x;
    vec4 weightedLayer_1 = layer1Sampler * layer_pct.y;
    vec4 weightedLayer_2 = layer2Sampler * layer_pct.z;
    vec4 weightedLayer_3 = layer3Sampler * layer_pct.w;

    vec4 weightedNormal_0 = normal0Sampler * layer_pct.x;
    vec4 weightedNormal_1 = normal1Sampler * layer_pct.y;
    vec4 weightedNormal_2 = normal2Sampler * layer_pct.z;
    vec4 weightedNormal_3 = normal3Sampler * layer_pct.w;

    // These are used later in the shader. left in to emphasise that different layers contribute to different blends
    float metalBlend = weightedLayer_0.a + weightedLayer_1.a;
    float specBlend = weightedLayer_2.a + weightedLayer_3.a;

    // And combine weighted layers with vertex color and a constant factor to have the final diffuse layer
    vec3 colorMapValue = colorSampler.rgb;// * clamp(colorSampler.a, 0.5, 1.0);
    if (!terrainParams.enableColorMap)
        colorMapValue = vec3(0.5, 0.5, 0.5);
    vec3 matDiffuse = (weightedLayer_0.rgb + weightedLayer_1.rgb + weightedLayer_2.rgb + weightedLayer_3.rgb) * colorMapValue * 2.0;
    albedo = matDiffuse.rgb;
    vec3 normal = (weightedNormal_0.rgb + weightedNormal_1.rgb + weightedNormal_2.rgb + weightedNormal_3.rgb);

    normal = fs_in.Normal;

    float depth = abs(fs_in.EyeSpacePosition.z / fs_in.EyeSpacePosition.w);

    if (envParams.sunParams.isEnabled)
    {
        vec3 lightDir = normalize(envParams.sunParams.direction);
        float diffuse = max(dot(lightDir, normal), 0.0);

        albedo = diffuse * envParams.sunParams.color * matDiffuse.rgb;
    }

    if (envParams.isEnabled)
    {
        vec3 ambient = envParams.ambientColor * matDiffuse.rgb;

        albedo += ambient;
    }

    if (tEditorParams.brushParams.isEnabled)
    {
        float d = dist(tEditorParams.brushParams.position.xz, fs_in.FragPos.xz) * (1.0 / tEditorParams.brushParams.size);
        vec3 brushColor = vec3(0.1, 0.1, 0.7);

        if (tEditorParams.brushParams.mode == 0)
        {
            vec3 brushValue = mix(brushColor, vec3(0.0, 0.0, 0.0), clamp(d, 0.0, 1.0));
            albedo += brushValue;
        }
        else if (tEditorParams.brushParams.mode == 1)
        {
            float minDist = 0.99;
            float maxDist = minDist + (tEditorParams.brushParams.size * 0.0002);//0.999;
            if (d > minDist && d < maxDist)
            {
                albedo += 1.0.xxx;
                //albedo = clamp(albedo, 0.0, 1.0);
            }
        }
    }

    // Draw Chunk edges
    if(tEditorParams.enableAreaGrid)
    {
        float cthickness = 0.001;
        vec2 wposMod = vec2(mod(32768.0 + fs_in.FragPos.x, 512.0), mod(32768.0 + fs_in.FragPos.z, 512.0)) / 512.0;
        if (wposMod.x < cthickness || wposMod.x > 1 - cthickness || wposMod.y < cthickness || wposMod.y > 1 - cthickness)
        {
            albedo = mix(vec3(0.3, 0.3, 1.0), albedo.rgb, clamp(depth / 800, 0, 1));
        }
    }

    // Draw SubChunk edges
    if(tEditorParams.enableChunkGrid)
    {
        float sthickness = 0.006;
        if (uv.x < sthickness || uv.x > 1 - sthickness || uv.y < sthickness || uv.y > 1 - sthickness)
        {
            albedo.rgb = mix(clamp(albedo.rgb + 0.3, 0, 1), albedo.rgb, clamp(depth / 800.0, 0, 1));
        }
    }

    // Apply fog calculation only if fog is enabled
    if(envParams.fogParams.isEnabled)
    {
        albedo = mix(albedo, envParams.fogParams.color, getFogFactor(envParams.fogParams, depth));
    }

    FragColor = vec4(albedo.rgb, 1.0);
    //FragColor = vec4(fs_in.Normal, 1.0);
    
}