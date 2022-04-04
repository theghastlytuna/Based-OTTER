#version 440

// Include our common vertex shader attributes and uniforms
#include "../fragments/vs_uncommon.glsl"

struct Material {
	sampler2D Diffuse;
	float     Shininess;
};
uniform Material u_Material;
uniform sampler1D s_1Dtex;

uniform float t;

void main() {

// Lecture 5
	// Pass vertex pos in world space to frag shader
	outViewPos = (u_ModelView * mix(vec4(inPosition, 1.0), vec4(inPosition2, 1.0), t)).xyz;

	// Normals
	outNormal = mat3(u_NormalMatrix) * mix(inNormal, inNormal2, t);

	// Pass our UV coords to the fragment shader
	outUV = inUV;

	///////////
	outColor = inColor;

	gl_Position = u_ModelViewProjection *  mix(vec4(inPosition, 1.0), vec4(inPosition2, 1.0), t);

}