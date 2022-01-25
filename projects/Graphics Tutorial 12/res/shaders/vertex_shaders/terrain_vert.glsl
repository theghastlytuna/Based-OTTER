#version 440

// Include our common vertex shader attributes and uniforms
#include "../fragments/vs_common.glsl"
#include "../fragments/math_constants.glsl"

// For more detailed explanations, see
// https://learnopengl.com/Advanced-Lighting/Normal-Mapping

uniform sampler2D s_Heightmap;
uniform float u_Scale;

layout(location = 7) out vec2 outTexWeights;

void main() {
    
    // Read our displacement value from the texture and apply the scale
    float displacement = textureLod(s_Heightmap, inUV, 0).r * u_Scale;
    // We'll use our surface normal for the dispalcement. We could use a normal map,
    // but this should give us OK results. Note that our displacement will be in
    // object space
    vec3 displacedPos = inPosition + (inNormal * displacement);

    // Transform to world position
	gl_Position = u_ModelViewProjection * vec4(displacedPos, 1.0);

	// Pass vertex pos in world space to frag shader
	outWorldPos = (u_Model * vec4(displacedPos, 1.0)).xyz;

	// Normals
	outNormal = inNormal;

	// Pass our UV coords to the fragment shader
	outUV = inUV;

	///////////
	outColor = inColor;


}