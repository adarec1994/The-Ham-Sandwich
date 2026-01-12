varying vec3 vertexNormal;
varying vec2 tex0;

const vec3 sunDirection = vec3(1, -1, -1);
const vec3 diffuseLight = vec3(0.7, 0.7, 0.7);

uniform sampler2D _texture0;

vec3 getDiffuseLight() {
	float light = dot(vertexNormal, -normalize(sunDirection));
	if(light < 0.0)
		light = 0.0;
	if(light > 0.5)
		 light = 0.5 + (light - 0.5) * 0.65;
		
	vec3 diffuse = diffuseLight * light;
	diffuse += vec3(0.4, 0.4, 0.4);
	diffuse = clamp(diffuse, 0.0, 1.0);

	return diffuse;
}

void main() {
	gl_FragColor = texture(_texture0, tex0); //vec4(getDiffuseLight(), 1.0);
}