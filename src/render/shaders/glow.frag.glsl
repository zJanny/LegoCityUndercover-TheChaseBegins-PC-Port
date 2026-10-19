#version 460 core
in vec2 texel;
uniform sampler2D image;
uniform int pass;
uniform vec2 tapOffset;
uniform float centreWeight;
uniform float sideWeight;
uniform float compositeColour;
uniform float compositeAlpha;
uniform float blurredAlpha;
uniform float compositeScale;
out vec4 colour;
void main() {
    if (pass == 0) {
        vec3 sum = texture(image, texel).rgb * centreWeight;
        sum = clamp(texture(image, texel + tapOffset).rgb * sideWeight + sum, 0.0, 1.0);
        sum = clamp(texture(image, texel - tapOffset).rgb * sideWeight + sum, 0.0, 1.0);
        colour = vec4(sum, 0.0);
    } else {
        vec3 glow = texture(image, texel).rgb;
        vec3 stage0 = clamp(vec3(compositeAlpha * blurredAlpha + compositeColour), 0.0, 1.0);
        vec3 stage1 = stage0 * glow;
        colour = vec4(clamp(stage1 * glow * compositeScale, 0.0, 1.0), 0.0);
    }
}
