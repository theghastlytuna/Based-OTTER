#version 430

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec3 outColor;

uniform layout(binding = 0) sampler2D s_Image;
uniform layout(binding = 1) sampler2D s_BrightImage;

//uniform float u_Filter[9];
//uniform vec2 u_PixelSize;

void main() {
    vec3 sceneCol = texture(s_Image, inUV).rgb;
    vec3 brightCol = texture(s_BrightImage, inUV).rgb;

    float exposure = 0.8f;

    vec3 toneMapped = vec3(1.0f) - exp(-sceneCol * exposure);

    outColor = sceneCol + brightCol;
}
