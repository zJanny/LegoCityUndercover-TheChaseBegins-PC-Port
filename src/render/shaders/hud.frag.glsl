#version 460 core
in vec2 texel;
uniform sampler2D image;
out vec4 colour;
void main() { colour = texture(image, texel); }
