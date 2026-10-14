#version 460 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 colour;
layout(location = 4) in vec2 uv1;
uniform mat4 mvp;
out vec2 vUv; out vec2 vUv1; out vec3 vNormal; out vec4 vColour; out float vViewDepth;
void main() { vUv = uv; vUv1 = uv1; vNormal = normal; vColour = colour; gl_Position = mvp * vec4(position, 1.0); vViewDepth = gl_Position.w; }
