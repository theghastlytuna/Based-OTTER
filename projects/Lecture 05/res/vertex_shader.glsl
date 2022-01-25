#version 410
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
//LEC 5
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outColor;
layout(location = 2) out vec3 outNormal;

//Lecture 4
uniform mat4 MVP;

uniform mat4 Model;



void main() {
	// vertex position in clip space
	gl_Position = MVP * vec4(inPosition, 1.0);

	//lec 5 -- vertex pos and normals in world space --> frag shader
	outPos = (Model * vec4(inPosition, 1.0)).xyz;
	outNormal = (Model * vec4(inNormal, 1.0)).xyz;

	outColor = inColor;
}


	