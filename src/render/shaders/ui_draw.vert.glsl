#version 460 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec4 tint;
uniform vec2 windowSize;
out vec4 vertexColour;
void main() {
    vertexColour = tint;
    gl_Position = vec4(position.x / windowSize.x * 2.0 - 1.0, 1.0 - position.y / windowSize.y * 2.0, 0.0, 1.0);
}
