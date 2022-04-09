#version 440

// Include our common vertex shader attributes and uniforms
#include "../fragments/vs_uncommon.glsl"

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

	vec2 grid = vec2(640, 240) * 0.5f;
	vec4 vertInClipSpace = u_ModelViewProjection * vec4(inPosition, 1.0);
	vec4 snapped = vertInClipSpace;
	snapped.xyz = vertInClipSpace.xyz / vertInClipSpace.w;
	snapped.xy = floor(grid * snapped.xy) / grid;
	snapped.xyz *= vertInClipSpace.w;

	vec4 vertInClipSpace2 = u_ModelViewProjection * vec4(inPosition2, 1.0);
	vec4 snapped2 = vertInClipSpace2;
	snapped2.xyz = vertInClipSpace2.xyz / vertInClipSpace2.w;
	snapped2.xy = floor(grid * snapped2.xy) / grid;
	snapped2.xyz *= vertInClipSpace2.w;

	gl_Position = mix(snapped, snapped2, t);

}