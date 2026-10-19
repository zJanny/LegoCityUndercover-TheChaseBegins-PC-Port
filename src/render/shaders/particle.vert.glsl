#version 460 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 colour;
uniform mat4 mvp;
out vec2 vUv;
out vec4 vColour;
out float vViewDepth;
void main() {
    vUv = uv;
    vColour = colour;
    gl_Position = mvp * vec4(position, 1.0);
    vViewDepth = gl_Position.w;
}
