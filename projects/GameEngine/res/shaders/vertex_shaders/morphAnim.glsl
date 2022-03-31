#version 440

// Include our common vertex shader attributes and uniforms
#include "../fragments/vs_uncommon.glsl"
#include "../fragments/multiple_point_lights.glsl"

struct Material {
	sampler2D Diffuse;
	float     Shininess;
};
uniform Material u_Material;

uniform float t;

void main() {

	// Lecture 5
	// Pass vertex pos in world space to frag shader
	outViewPos = (u_ModelView * vec4(inPosition, 1.0)).xyz;

	// Normals
	outNormal = (u_View * vec4(mat3(u_NormalMatrix) * inNormal, 0)).xyz;

	// Pass our UV coords to the fragment shader
	outUV = inUV;

	///////////
	outColor = inColor;

	outLight = CalcAllLightContribution(outViewPos, outNormal, u_CamPos.xyz, u_Material.Shininess);

	gl_Position = u_ModelViewProjection *  mix(vec4(inPosition, 1.0), vec4(inPosition2, 1.0), t);

}