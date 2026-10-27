#version 460 core
layout(location = 0) in vec3 position;
uniform mat4 mvp;
uniform mat4 shadowMvp;
out vec4 vShadow;
void main() {
    vShadow = shadowMvp * vec4(position, 1.0);
    gl_Position = mvp * vec4(position, 1.0);
}
