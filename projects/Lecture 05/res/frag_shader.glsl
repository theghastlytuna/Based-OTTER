#version 410

layout(location = 1) in vec3 inColor;

layout(location = 0) in vec3 inPos;
layout(location = 2) in vec3 inNormal;

uniform vec3 lightPos;
uniform vec3 cameraPos;

out vec4 frag_color;


void main() { 
	
	//lec5 ambient
	vec3 lightColor = vec3(1.0, 1.0, 1.0);
	float ambientStrength = 0.1;
	vec3 ambient = ambientStrength * lightColor * inColor;

	//diffuse
	vec3 N = normalize(inNormal);
	vec3 lightDir = normalize(lightPos - inPos);

	float d = max(dot(N, lightDir), 0.0);
	vec3 diffuse = d * inColor;

	//attenuation
	float dist = length(lightPos - inPos);
	diffuse = diffuse / dist*dist;

	//specular
	float specularStrength = 0.6;
	vec3 cameraDir = normalize(cameraPos - inPos);//v
	//vec3 reflectedRay = reflect(-lightDir, N);//r
	vec3 halfway = normalize(lightDir + cameraDir);
	//float spec = pow(max(dot(cameraDir, reflectedRay), 0.0), 128);
	float spec = pow(max(dot(N, halfway), 0.0), 128);
	vec3 specular = specularStrength * spec * lightColor;

	vec3 result = ambient + diffuse + specular;

	frag_color = vec4(result, 1.0);
}