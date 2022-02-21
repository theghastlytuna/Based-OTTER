#version 440

// Include our common vertex shader attributes and uniforms
#include "../fragments/vs_uncommon.glsl"
#include "../fragments/multiple_point_lights.glsl"

uniform float t;

struct Material {
	sampler2D Diffuse;
	float     Shininess;
};
// Create a uniform for the material
uniform Material u_Material;

void main() {

	vec2 grid = vec2(427, 240) * 0.5f;
	vec4 vertInClipSpace = u_ModelViewProjection *  mix(vec4(inPosition, 1.0), vec4(inPosition2, 1.0), t);
	vec4 snapped = vertInClipSpace;
	snapped.xyz = vertInClipSpace.xyz / vertInClipSpace.w;
	snapped.xy = floor(grid * snapped.xy) / grid;
	snapped.xyz *= vertInClipSpace.w;

	gl_Position = snapped;

	// Lecture 5
	// Pass vertex pos in world space to frag shader
	outWorldPos = (u_Model * mix(vec4(inPosition, 1.0), vec4(inPosition2, 1.0), t)).xyz;

	// Normals
	outNormal = mat3(u_NormalMatrix) * mix(inNormal, inNormal2, t);

	// Pass our UV coords to the fragment shader
	outUV = inUV;

	///////////
	outColor = inColor;

	//outLight = CalcAllLightContribution(snapped.xyz, outNormal, u_CamPos.xyz, u_Material.Shininess);
}