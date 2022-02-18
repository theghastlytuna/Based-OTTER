#version 430

#include "../fragments/fs_common_inputs.glsl"
#include "../fragments/frame_uniforms.glsl"
#include "../fragments/multiple_point_lights.glsl"

struct Material {
	sampler2D DiffuseA;
	sampler2D DiffuseB;
	float     Shininess;
};

// We output a single color to the color buffer
layout(location = 0) out vec4 frag_color;

// Create a uniform for the material
uniform Material u_Material;

void main() {
	vec3 normal = normalize(inNormal);

	// Get the albedo from the diffuse / albedo map
	vec4 textureColor = texture(u_Material.Diffuse, inUV);

	// combine for the final result
	vec3 result = inLight * inColor * textureColor.rgb;

	frag_color = vec4(result, textureColor.a);
}