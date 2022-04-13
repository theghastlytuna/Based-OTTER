#version 430

// Based on the Unity shader found at
// https://roystan.net/articles/outline-shader.html

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inViewDir;

layout (location = 0) out vec3 outColor;

uniform layout(binding = 0) sampler2D s_Image;
uniform layout(binding = 1) sampler2D s_Depth;
uniform layout(binding = 2) sampler2D s_Normals;
uniform layout(binding = 3) sampler2D u_NoiseTex;

#include "../../fragments/frame_uniforms.glsl"

void main() {
    vec2 uv;
    uv.x = 0.4 * sin(u_Time * 50);                                 
    uv.y = 0.4 * cos(u_Time * 50);       
    vec3 image = texture(s_Image, inUV).rgb;
    vec3 visionColor = vec3(0.1, 0.95, 0.2);
    vec3 n = texture(u_NoiseTex, inUV + uv).rgb;

    float lum = dot(vec3(0.30, 0.59, 0.11), image);
    if (lum < 0.8)
      image *= 4.0; 

    outColor.rgb = (image + n * 0.2) * visionColor;
}
