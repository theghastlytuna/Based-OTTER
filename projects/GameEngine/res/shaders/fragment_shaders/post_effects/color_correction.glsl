#version 430
#include "frame_uniforms.glsl"

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec3 outColor;

uniform layout(binding = 0) sampler2D s_Image;
uniform layout(binding = 1) sampler3D s_WarmLut;
uniform layout(binding = 2) sampler3D s_CoolLut;
uniform layout(binding = 3) sampler3D s_CustomLut;

uniform float u_Strength;
uniform float u_Selection;

void main() {
    vec3 color = texture(s_Image, inUV).rgb;

    if (u_Selection >= 2) 
    {
        outColor = mix(color, texture(s_CustomLut, color).rgb, clamp(u_Strength, 0, 1));
    }
    else if (u_Selection >= 1) 
    {
        outColor = mix(color, texture(s_CoolLut, color).rgb, clamp(u_Strength, 0, 1));
    }
    else if (u_Selection >= 0) 
    {
        outColor = mix(color, texture(s_WarmLut, color).rgb, clamp(u_Strength, 0, 1));
    }
    else if (u_Selection < 0)
    {
        outColor = color;
    }

}

