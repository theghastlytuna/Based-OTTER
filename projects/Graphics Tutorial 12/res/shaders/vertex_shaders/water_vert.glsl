#version 440

// Include our common vertex shader attributes and uniforms
#include "../fragments/vs_common.glsl"
#include "../fragments/math_constants.glsl"

// For more detailed explanations, see
// https://learnopengl.com/Advanced-Lighting/Normal-Mapping

uniform float u_Scale;

void main() {

	// Pass vertex pos in world space to frag shader
	vec3 vert = inPosition;
	vert.z += sin(vert.x * u_Scale + u_Time) * 0.02;

	outWorldPos = (u_Model * vec4(vert, 1.0)).xyz;

	gl_Position = u_ModelViewProjection * vec4(vert, 1.0);

	// Normals
	outNormal = mat3(u_NormalMatrix) * inNormal;

	// Pass our UV coords to the fragment shader
	outUV = inUV;

	///////////
	outColor = inColor;
}