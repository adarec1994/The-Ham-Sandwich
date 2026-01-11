attribute vec3 position0;
attribute vec3 normal0;
attribute vec2 texcoord0;

uniform mat4 matView;
uniform mat4 matProj;
uniform mat4 matWorld;

varying vec2 tex0;
varying vec3 vertexNormal;

void main(void) {
	tex0 = texcoord0;
	vertexNormal = normalize((matWorld * vec4(normal0, 1)).xyz);
	gl_Position = matProj * matView * matWorld * vec4(position0, 1);
}