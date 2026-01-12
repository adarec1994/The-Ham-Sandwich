#version 420 core
out vec4 FragColor;

layout (binding=0) uniform sampler2D u_texture;

uniform vec4 lineColor;
uniform float aspectRatio;

in VS_OUT 
{
    vec3 FragPos;
    vec2 UV;
    vec2 screenPos;
} fs_in;

void main()
{
	//vec2 uv = fs_in.UV;
    vec2 screenPos = fs_in.screenPos;
    float lineSize = 1;
    float xG = fract(screenPos.x * aspectRatio);
    float yG = fract(screenPos.y);

    vec2 texel = vec2(xG, yG);
    //float d = fract(texel.x + texel.y);

    float scl = 50.0; // how many ants

    float c = sin(texel.x * scl - texel.y * scl);

    if (c < 0.1)
    {
        FragColor = lineColor;
    }
    else
    {
        discard;
    }

    /*
    vec2 uv = vec2(screenPos * lineSize);
    uv.x *= aspectRatio;

    float scl = 50.0; // how many ants
    float smoothing = 0.2; // smoothing at the tips of each dash
    float lineW = 0.01; // line thickness

    float x = xG;
    float y = yG;
    
    float c = sin(uv.x * scl + uv.y * scl);
    c = smoothstep(-smoothing,smoothing,c);

    float m = max(uv.x, uv.y);
    c = mix(1.0, c, step(m, 0.0)); // outside
    c = mix(c, 1.0, step(m, -lineW)); // inside   

	// Output color = color of the texture at the specified UV
	//uv.y = 1.0 - uv.y;
	//FragColor = texture( u_texture, uv );
	//FragColor.a *= 1.0 - smoothstep(0.0, 500.0, fs_in.dist);
	FragColor = vec4(1.0 - c);
    */
}