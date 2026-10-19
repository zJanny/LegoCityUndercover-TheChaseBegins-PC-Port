#version 460 core
in vec2 vUv;
in vec4 vColour;
in float vViewDepth;
uniform sampler2D diffuse;
uniform bool alphaTest;
uniform float alphaReference;
uniform bool fogOn; uniform vec3 fogColour; uniform vec2 fogProjection; uniform uint fogLut[128];
out vec4 fragment;
vec3 applyFog(vec3 colour) {
    if (!fogOn) return colour;
    float depthScale = fogProjection.y / (fogProjection.y - fogProjection.x);
    float depth = clamp(depthScale * (1.0 - fogProjection.x / vViewDepth), 0.0, 1.0);
    float position = depth * 128.0;
    int index = min(int(floor(position)), 127);
    uint word = fogLut[index];
    int difference = int(word & 0x1fffu);
    if (difference >= 4096) difference -= 8192;
    float factor = clamp((float((word >> 13) & 0x7ffu) + float(difference) * (position - float(index))) / 2048.0, 0.0, 1.0);
    return mix(fogColour, colour, factor);
}

void main() {
    vec4 colour = texture(diffuse, vUv) * vColour;
    if (alphaTest && colour.a * 255.0 <= alphaReference) discard;
    fragment = vec4(applyFog(colour.rgb), colour.a);
}
