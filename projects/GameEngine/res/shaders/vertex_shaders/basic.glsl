#version 440

// Include our common vertex shader attributes and uniforms
#include "../fragments/vs_common.glsl"
#include "../fragments/multiple_point_lights.glsl"

struct Material {
	sampler2D Diffuse;
	float     Shininess;
};
uniform Material u_Material;
uniform sampler1D s_1Dtex;

void main() {

	vec2 grid = vec2(427, 240) * 0.5f;
	vec4 vertInClipSpace = u_ModelViewProjection * vec4(inPosition, 1.0);
	vec4 snapped = vertInClipSpace;
	snapped.xyz = vertInClipSpace.xyz / vertInClipSpace.w;
	snapped.xy = floor(grid * snapped.xy) / grid;
	snapped.xyz *= vertInClipSpace.w;
	vec3 worldPos = (u_Model * vec4(inPosition, 1.0)).xyz;
	
	vec3 tempNormal = mat3(u_NormalMatrix) * inNormal;

	gl_Position = snapped;

	// Lecture 5
	// Pass vertex pos in world space to frag shader
	outViewPos = (u_ModelView * vec4(inPosition, 1.0)).xyz;

	// Normals
	outNormal = (u_View * vec4(mat3(u_NormalMatrix) * inNormal, 0)).xyz;

    // We use a TBN matrix for tangent space normal mapping
    vec3 T = normalize((u_View * vec4(mat3(u_NormalMatrix) * inTangent, 0)).xyz);
    vec3 B = normalize((u_View * vec4(mat3(u_NormalMatrix) * inBiTangent, 0)).xyz);
    vec3 N = normalize((u_View * vec4(mat3(u_NormalMatrix) * inNormal, 0)).xyz);
    mat3 TBN = mat3(T, B, N);

    // We can pass the TBN matrix to the fragment shader to save computation
    outTBN = TBN;

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
		outLight = CalcSpec(outViewPos, tempNormal, u_CamPos.xyz, u_Material.Shininess);
	}

	else if (IsFlagSet(FLAG_ENABLE_AMBSPEC))
	{
		outLight = CalcSpec(outViewPos, tempNormal, u_CamPos.xyz, u_Material.Shininess) + CalcAmbient();
	}
	
	/*
	else if (IsFlagSet(FLAG_ENABLE_CUSTOMSHADER))
	{
		//Emboss shader concept from https://www.raywenderlich.com/2941-how-to-create-cool-effects-with-custom-shaders-in-opengl-es-2-0-and-cocos2d-2-x
		
		//Size of one texel
		vec2 onePixel = vec2(1.0 / 1200.0, 1.0 / 800.0);

		//Create a temporary colour vector, make it grey
		vec4 tempColour;
		tempColour.rgb = vec3(0.5);

		//Add colour from both sides of the texel. This means that any time a drastic change in colour occurs, it's emphasized by the emboss effect.
		//Multiply by a scalar to get a more intense effect
		tempColour -= texture2D(u_Material.Diffuse, inUV - onePixel) * 8.0;
		tempColour += texture2D(u_Material.Diffuse, inUV + onePixel) * 8.0;
		
		//Average the colours to keep them grey
		tempColour.rgb = vec3((tempColour.r + tempColour.g + tempColour.b) / 3.0);

		//Multiply by lightAccumulation to keep the lighting in
		vec3 lightAccumulation = CalcAllLightContribution(inWorldPos, normal, u_CamPos.xyz, u_Material.Shininess);
		frag_color = vec4(tempColour.rgb * lightAccumulation, 1);
	}
	
	else if (IsFlagSet(FLAG_ENABLE_DIFFUSEWARP))
	{
		// combine for the final result
		vec3 result;

		// Using LUT
		result.r = texture(s_1Dtex, textureColor.r).r;
		result.g = texture(s_1Dtex, textureColor.g).g;
		result.b = texture(s_1Dtex, textureColor.b).b;

		frag_color = vec4(result, textureColor.a);
	}
	*/

	else if (IsFlagSet(FLAG_ENABLE_SPECWARP))
	{
		// Use the lighting calculation that we included from our partial file
		vec3 lightAccumulation = CalcSpec(outViewPos, tempNormal, u_CamPos.xyz, u_Material.Shininess);

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
		outLight = CalcAllLightContribution(outViewPos, tempNormal, u_CamPos.xyz, u_Material.Shininess);
	}

	//outLight = CalcAllLightContribution(outViewPos, outNormal, u_CamPos.xyz, u_Material.Shininess);
}