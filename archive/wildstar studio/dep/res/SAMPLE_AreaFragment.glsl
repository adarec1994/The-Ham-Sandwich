varying vec2 texcoord;

varying vec3 normal;
varying vec3 halfVector;
varying vec3 lightDir;

uniform sampler2D alphaTexture;
uniform sampler2D texture0;
uniform sampler2D normalTexture0;
uniform sampler2D texture1;
uniform sampler2D normalTexture1;
uniform sampler2D texture2;
uniform sampler2D normalTexture2;
uniform sampler2D texture3;
uniform sampler2D normalTexture3;
uniform sampler2D colorTexture;
uniform float hasColorMap;
uniform vec4 texScale;

const vec3 sunDirection = vec3(1, 1, -1);

const vec3 diffuseLight = vec3(0.8, 0.8, 0.8);

vec4 getTextureColor(vec4 color, vec4 nm) {
	vec3 l = normalize(-sunDirection);//lightDir);
	vec3 n = normalize(nm.rgb);
	vec3 h = normalize(halfVector);

	float nDotL = clamp(dot(n, l), 0.0, 1.0);
	float nDotH = clamp(dot(n, h), 0.0, 1.0);

	vec3 lcolor = vec3(0.3, 0.3, 0.3) + vec3(1, 1, 1) * nDotL;
	vec4 ret = color;
	ret.rgb *= lcolor;
	return ret;
}

float getLightFactor() {
	float light = dot(normal, -normalize(sunDirection));
	if(light < 0.0)
		light = 0.0;
	if(light > 0.5)
		 light = 0.5 + (light - 0.5) * 0.65;

	return light;
}

vec3 getDiffuseLight() {
	float light = dot(normal, -normalize(sunDirection));
	if(light < 0.0)
		light = 0.0;
	if(light > 0.5)
		 light = 0.5 + (light - 0.5) * 0.65;
		
	vec3 diffuse = diffuseLight * light;
	diffuse += vec3(0.2, 0.2, 0.2);
	diffuse = clamp(diffuse, 0.0, 1.0);

	return diffuse;
}

void main() {
	vec4 alpha = texture(alphaTexture, texcoord);

	vec4 tex1 = texture(texture0, texcoord * texScale.x);
	vec4 norm1 = texture(normalTexture0, texcoord * texScale.x);
	vec4 tex2 = texture(texture1, texcoord * texScale.y);
	vec4 norm2 = texture(normalTexture1, texcoord * texScale.y);
	vec4 tex3 = texture(texture2, texcoord * texScale.z);
	vec4 norm3 = texture(normalTexture2, texcoord * texScale.z);
	vec4 tex4 = texture(texture3, texcoord * texScale.w);
	vec4 norm4 = texture(normalTexture3, texcoord * texScale.w);

	//tex1 = getTextureColor(tex1, norm1);
	//tex2 = getTextureColor(tex2, norm2);
	//tex3 = getTextureColor(tex3, norm3);
	//tex4 = getTextureColor(tex4, norm4);

	vec4 clr = alpha.r * tex1 + alpha.g * tex2 + alpha.b * tex3 + alpha.a * tex4;

	if(hasColorMap > 0.5) {
		vec4 texClr = texture(colorTexture, texcoord);
		clr.rgb *= texClr.rgb * 2;
	}

	clr.rgb *= getDiffuseLight();

	gl_FragColor = clr;
}