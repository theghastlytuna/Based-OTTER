#version 430

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec3 outColor;
layout(location = 1) out vec3 outColor2;

uniform layout(binding = 0) sampler2D s_Image;
uniform layout(binding = 1) sampler2D s_noiseTex;

#include "../../fragments/frame_uniforms.glsl"

void main() {
    vec2 uv;
    uv.x = 0.4 * sin(u_Time * 50);                                 
    uv.y = 0.4 * cos(u_Time * 50); 

    vec3 n = texture(s_noiseTex, inUV + uv).rgb;

    outColor.rgb = (texture(s_Image, inUV).rgb + (n * 0.2));

}
