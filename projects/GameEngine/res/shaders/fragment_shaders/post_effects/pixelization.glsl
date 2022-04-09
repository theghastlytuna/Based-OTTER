#version 430

// Based on the Unity shader found at
// https://roystan.net/articles/outline-shader.html

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inViewDir;

layout (location = 0) out vec3 outColor;


uniform layout(binding = 0) sampler2D s_Image;
uniform layout(binding = 1) sampler2D s_Depth;
uniform layout(binding = 2) sampler2D s_Normals;

uniform float u_Power;

#include "../../fragments/frame_uniforms.glsl"

void main() {

    vec2 newUV;
    newUV.s = floor(inUV.s * 640) / 640;
    newUV.t = floor(inUV.t * 480) / 480;

    vec3 color1 = texture(s_Image, newUV).rgb;

    vec3 color2 = texture(s_Image, inUV).rgb;

    vec3 result = mix(color2, color1, u_Power);

    outColor = vec3(result);
}
