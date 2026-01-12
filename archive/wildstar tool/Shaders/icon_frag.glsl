#version 420 core
out vec4 FragColor;

layout (binding=0) uniform sampler2D u_texture;

in VS_OUT 
{
    vec2 UV;
	float dist;
} fs_in;

void main()
{
	// Output color = color of the texture at the specified UV
	vec2 uv = fs_in.UV;
	uv.y = 1.0 - uv.y;
	FragColor = texture( u_texture, uv );
	FragColor.a *= 1.0 - smoothstep(0.0, 500.0, fs_in.dist);
}