#version 460 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 colour;
layout(location = 4) in vec2 uv1;
uniform mat4 mvp;
uniform mat4 model;
uniform bool cubeOn; uniform vec3 eyePosition;
out vec3 vCube;
out vec2 vUv; out vec2 vUv1; out vec3 vNormal; out vec4 vColour; out float vViewDepth; out vec3 vWorldNormal; out vec3 vWorldPosition;
void main() {
    vUv = uv; vUv1 = uv1; vNormal = normal; vColour = colour;
    vWorldNormal = mat3(model) * normal;
    vWorldPosition = (model * vec4(position, 1.0)).xyz;
    gl_Position = mvp * vec4(position, 1.0); vViewDepth = gl_Position.w;
    vCube = vec3(0.0);
    if (cubeOn) {
        vec3 objectEye = transpose(mat3(model)) * (eyePosition - model[3].xyz);
        vec3 n = normalize(normal);
        vec3 v = normalize(objectEye - position);
        vCube = mat3(model) * (2.0 * dot(v, n) * n - v);
    }
}
