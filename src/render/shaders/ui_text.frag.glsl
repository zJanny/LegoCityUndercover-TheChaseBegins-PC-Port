#version 460 core
in vec2 texel;
in vec4 colour;
uniform sampler2D atlas;
out vec4 result;
void main() { result = vec4(colour.rgb, colour.a * texture(atlas, texel).r); }
