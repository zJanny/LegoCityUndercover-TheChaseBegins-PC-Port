#version 460 core
in vec2 vUv;
in vec4 vColour;
uniform sampler2D diffuse;
uniform bool alphaTest;
uniform float alphaReference;
out vec4 fragment;
void main() {
    vec4 colour = texture(diffuse, vUv) * vColour;
    if (alphaTest && colour.a * 255.0 <= alphaReference) discard;
    fragment = colour;
}
