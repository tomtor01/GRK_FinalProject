#version 430 core

float AMBIENT = 0.1;

uniform vec3 color;
uniform vec3 lightPos;
uniform sampler2D colorTexture;

uniform sampler2D textureSampler;
uniform sampler2D normalSampler;

in vec3 viewDirTS;
in vec3 lightDirTS;

in vec3 vecNormal;
in vec3 worldPos;
in vec2 vecTex;
in mat3 TBN;

out vec4 outColor;

void main()
{

	vec3 lightDir = normalize(lightDirTS);
	//vec3 lightDir = normalize(lightPos-worldPos);
	vec3 viewDir = normalize(viewDirTS);
	//vec3 normal = vec3(0,0,1);

	vec3 textureColor = texture2D(colorTexture, vecTex).xyz;
	vec3 N = texture2D(normalSampler, vecTex).xyz;
	N = N*2.0 - 1.0;
	N = normalize(N);
	float diffuse=max(0.001 ,dot(N,lightDir));
	outColor = vec4(textureColor*min(1,AMBIENT+diffuse), 1.0);
}
