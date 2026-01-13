attribute vec3 position0;
attribute vec3 normal0;
attribute vec4 normal1; // tangent
attribute vec2 texcoord0;

uniform mat4 matView;
uniform mat4 matProj;
uniform vec3 camPosition;

varying vec2 texcoord;
varying vec3 normal;
varying vec3 halfVector;
varying vec3 lightDir;

const vec3 sunDirection = vec3(1, 1, -1);

void main(void) {
	vec4 tangent = normal1;
	vec3 lightDir = sunDirection;
	normal = normal0;
	texcoord = texcoord0;
	vec3 viewDir = camPosition - position0;
	halfVector = normalize(normalize(lightDir) + normalize(viewDir));
	vec3 n = normal;
	vec3 t = tangent.xyz;
	vec3 b = cross(n, t) * tangent.w;
	mat3 tbn;
	tbn[0] = vec3(t.x, b.x, n.x);
	tbn[1] = vec3(t.y, b.y, n.y);
	tbn[2] = vec3(t.z, b.z, n.z);
	halfVector = tbn * halfVector;
	lightDir = tbn * lightDir;
	gl_Position = matProj * matView * vec4(position0, 1);
}