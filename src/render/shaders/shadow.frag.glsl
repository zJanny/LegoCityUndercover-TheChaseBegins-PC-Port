#version 460 core
in vec4 vShadow;
uniform bool receiving;
uniform sampler2DShadow shadowTexture;
uniform vec3 shadowConstant;
out vec4 fragColour;
void main() {
    if (!receiving) {
        fragColour = vec4(1.0);
        return;
    }
    vec3 coordinate = vShadow.xyz / vShadow.w * 0.5 + 0.5;
    float lit = 1.0;
    if (all(greaterThanEqual(coordinate, vec3(0.0))) && all(lessThanEqual(coordinate, vec3(1.0))))
        lit = texture(shadowTexture, coordinate);
    fragColour = vec4(clamp(lit * shadowConstant + (1.0 - shadowConstant), 0.0, 1.0), 1.0);
}
