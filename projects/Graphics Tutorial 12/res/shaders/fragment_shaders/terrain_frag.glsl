#version 440

#include "../fragments/fs_common_inputs.glsl"

// We output a single color to the color buffer
layout(location = 0) out vec4 frag_color;

////////////////////////////////////////////////////////////////
/////////////// Frame Level Uniforms ///////////////////////////
////////////////////////////////////////////////////////////////

#include "../fragments/frame_uniforms.glsl"

////////////////////////////////////////////////////////////////
/////////////// Instance Level Uniforms ////////////////////////
////////////////////////////////////////////////////////////////

// Represents a collection of attributes that would define a material
// For instance, you can think of this like material settings in 
// Unity
struct Material {
	sampler2D DiffuseA;
	sampler2D DiffuseB;
	sampler2D DiffuseC;
	float     Shininess;
};
// Create a uniform for the material
uniform Material u_Material;

////////////////////////////////////////////////////////////////
///////////// Application Level Uniforms ///////////////////////
////////////////////////////////////////////////////////////////

#include "../fragments/multiple_point_lights.glsl"

const float LOG_MAX = 2.40823996531;

// https://learnopengl.com/Advanced-Lighting/Advanced-Lighting
void main() {
	// Normalize our input normal
	vec3 normal = normalize(inNormal);

	// Will accumulate the contributions of all lights on this fragment
	// This is defined in the fragment file "multiple_point_lights.glsl"
	vec3 lightAccumulation = CalcAllLightContribution(inWorldPos, normal, u_CamPos.xyz, u_Material.Shininess);

    // By we can use this lil trick to divide our weight by the sum of all components
    // This will make all of our texture weights add up to one! 
    //vec2 texWeight = inTextureWeights / dot(inTextureWeights, vec2(1,1));

	// Perform our texture mixing, we'll calculate our albedo as the sum of the texture and it's weight

	vec4 textureColor;
	vec3 lowerBound;
	vec3 upperBound;

	//sand
	if (inWorldPos.z < 0.8)
	{
		textureColor = texture(u_Material.DiffuseA, inUV);

	}
	//sand/grass
	else if (inWorldPos.z < 1.3)
	{
		lowerBound = texture(u_Material.DiffuseA,inUV).rgb;
		upperBound = texture(u_Material.DiffuseB,inUV).rgb;

		textureColor = vec4(mix(lowerBound, upperBound, (inWorldPos.z - 0.8) / 0.5), 1);
	}
	//grass
	else if (inWorldPos.z < 1.8)
	{
		textureColor = texture(u_Material.DiffuseB, inUV);
	}
	//grass/stone
	else if (inWorldPos.z < 2.3)
	{
		lowerBound = texture(u_Material.DiffuseB,inUV).rgb;
		upperBound = texture(u_Material.DiffuseC,inUV).rgb;

		textureColor = vec4(mix(lowerBound, upperBound, (inWorldPos.z - 1.8) / 0.5), 1);
	}
	else if (inWorldPos.z >= 2.3)
	{
		textureColor = texture(u_Material.DiffuseC, inUV);
	}

	vec3 result = inColor * textureColor.rgb;

	// combine for the final result

	frag_color = vec4(result, textureColor.a);
}