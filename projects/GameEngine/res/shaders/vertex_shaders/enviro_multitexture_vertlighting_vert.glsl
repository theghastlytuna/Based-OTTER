#version 440

// Include our common vertex shader attributes and uniforms
#include "../fragments/vs_common.glsl"
#include "../fragments/multiple_point_lights.glsl"

struct Material {
	sampler2D DiffuseA;
	sampler2D DiffuseB;
	float     Shininess;
};
// Create a uniform for the material
uniform Material u_Material;

void main() {

	vec2 grid = vec2(427, 240) * 0.5f;
	vec4 vertInClipSpace = u_ModelViewProjection * vec4(inPosition, 1.0);
	vec4 snapped = vertInClipSpace;
	snapped.xyz = vertInClipSpace.xyz / vertInClipSpace.w;
	snapped.xy = floor(grid * snapped.xy) / grid;
	snapped.xyz *= vertInClipSpace.w;

	gl_Position = snapped;

	// Lecture 5
	// Pass vertex pos in world space to frag shader
	outWorldPos = (u_Model * vec4(snapped.xyz, 1.0)).xyz;

	// Normals
	outNormal = mat3(u_NormalMatrix) * inNormal;

    // We use a TBN matrix for tangent space normal mapping
    vec3 T = normalize(vec3(mat3(u_NormalMatrix) * inTangent));
    vec3 B = normalize(vec3(mat3(u_NormalMatrix) * inBiTangent));
    vec3 N = normalize(vec3(mat3(u_NormalMatrix) * inNormal));
    mat3 TBN = mat3(T, B, N);

    // We can pass the TBN matrix to the fragment shader to save computation
    outTBN = TBN;

	// Pass our UV coords to the fragment shader
	outUV = inUV;

	///////////
	outColor = inColor;

	outLight = CalcAllLightContribution(outWorldPos, outNormal, u_CamPos.xyz, u_Material.Shininess);
}