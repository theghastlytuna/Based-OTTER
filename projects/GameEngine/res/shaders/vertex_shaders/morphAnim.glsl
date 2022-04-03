#version 440

// Include our common vertex shader attributes and uniforms
#include "../fragments/vs_uncommon.glsl"
#include "../fragments/multiple_point_lights.glsl"

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
	outViewPos = (u_ModelView * vec4(inPosition, 1.0)).xyz;

	// Normals
	outNormal = (u_View * vec4(mat3(u_NormalMatrix) * inNormal, 0)).xyz;

	// Pass our UV coords to the fragment shader
	outUV = inUV;

	///////////
	outColor = inColor;

	if (IsFlagSet(FLAG_DISABLE_LIGHTING)) 
	{
		outLight = vec3(1.0f, 1.0f, 1.0f);
	}

	else if (IsFlagSet(FLAG_ENABLE_AMBIENT))
	{
		outLight = CalcAmbient();
	}

	else if (IsFlagSet(FLAG_ENABLE_SPECULAR))
	{
		outLight = CalcSpec(outViewPos, outNormal, u_CamPos.xyz, u_Material.Shininess);
	}

	else if (IsFlagSet(FLAG_ENABLE_AMBSPEC))
	{
		outLight = CalcSpec(outViewPos, outNormal, u_CamPos.xyz, u_Material.Shininess) + CalcAmbient();
	}

	else if (IsFlagSet(FLAG_ENABLE_SPECWARP))
	{
		// Use the lighting calculation that we included from our partial file
		vec3 lightAccumulation = CalcSpec(outViewPos, outNormal, u_CamPos.xyz, u_Material.Shininess);

		// combine for the final result
		vec3 result;

		// Using a LUT to allow artists to tweak toon shading settings
		result.r = texture(s_1Dtex, lightAccumulation.r).r;
		result.g = texture(s_1Dtex, lightAccumulation.g).g;
		result.b = texture(s_1Dtex, lightAccumulation.b).b;

		outLight = result;
	}

	else
	{
		outLight = CalcAllLightContribution(outViewPos, outNormal, u_CamPos.xyz, u_Material.Shininess);
	}

	gl_Position = u_ModelViewProjection *  mix(vec4(inPosition, 1.0), vec4(inPosition2, 1.0), t);

}