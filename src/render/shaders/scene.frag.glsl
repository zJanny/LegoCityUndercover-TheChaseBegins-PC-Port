#version 460 core
in vec2 vUv; in vec2 vUv1; in vec3 vNormal; in vec4 vColour;
uniform sampler2D unit0; uniform sampler2D unit1; uniform sampler2D unit2;
uniform int stageCount; uniform ivec4 stageWords[6]; uniform vec4 stageConstant[6];
uniform int bufferBits; uniform int unitMode[3]; uniform vec4 unitFlat[3];
uniform bool hasNormal; uniform bool hasSecondUv; uniform bool dynamicLight; uniform float alphaReference; uniform float brightness;
uniform vec3 lightingStandIn;
out vec4 fragColour;

vec4 unitColour(int unit) {
    if (unitMode[unit] == 2) return unitFlat[unit];
    if (unitMode[unit] == 0) return vec4(1.0);
    vec2 uv = (unit == 0 || !hasSecondUv) ? vUv : vUv1;
    if (unit == 0) return texture(unit0, uv);
    if (unit == 1) return texture(unit1, uv);
    return texture(unit2, uv);
}
vec3 rgbOperand(vec4 c, int op) {
    switch (op) {
    case 0: return c.rgb;
    case 1: return 1.0 - c.rgb;
    case 2: return vec3(c.a);
    case 3: return vec3(1.0 - c.a);
    case 4: return vec3(c.r);
    case 5: return vec3(1.0 - c.r);
    case 8: return vec3(c.g);
    case 9: return vec3(1.0 - c.g);
    case 12: return vec3(c.b);
    default: return vec3(1.0 - c.b);
    }
}
float alphaOperand(vec4 c, int op) {
    switch (op) {
    case 0: return c.a;
    case 1: return 1.0 - c.a;
    case 2: return c.r;
    case 3: return 1.0 - c.r;
    case 4: return c.g;
    case 5: return 1.0 - c.g;
    case 6: return c.b;
    default: return 1.0 - c.b;
    }
}
vec4 source(int s, int stage, vec4 previous, vec4 bufferColour, vec4 fragPrimary) {
    if (s == 0) return vColour;
    if (s == 1) return fragPrimary;
    if (s == 2) return vec4(0.0);
    if (s == 3) return unitColour(0);
    if (s == 4) return unitColour(1);
    if (s == 5) return unitColour(2);
    if (s == 0xd) return bufferColour;
    if (s == 0xe) return stageConstant[stage];
    if (s == 0xf) return previous;
    return vec4(0.0);
}
vec3 combine3(int mode, vec3 a0, vec3 a1, vec3 a2) {
    switch (mode) {
    case 0: return a0;
    case 1: return a0 * a1;
    case 2: return a0 + a1;
    case 3: return a0 + a1 - 0.5;
    case 4: return a0 * a2 + a1 * (1.0 - a2);
    case 5: return a0 - a1;
    case 6: case 7: return vec3(4.0 * dot(a0 - 0.5, a1 - 0.5));
    case 8: return a0 * a1 + a2;
    default: return (a0 + a1) * a2;
    }
}
float combine1(int mode, float a0, float a1, float a2) {
    switch (mode) {
    case 0: return a0;
    case 1: return a0 * a1;
    case 2: return a0 + a1;
    case 3: return a0 + a1 - 0.5;
    case 4: return a0 * a2 + a1 * (1.0 - a2);
    case 5: return a0 - a1;
    case 8: return a0 * a1 + a2;
    default: return (a0 + a1) * a2;
    }
}
void main() {
    vec4 c;
    if (stageCount == 0) {
        c = vColour;
        c *= unitColour(0);
        if (c.a < 0.25) discard;
        float shade = 1.0;
        if (hasNormal && dot(vNormal, vNormal) > 0.0)
            shade = 0.45 + 0.55 * max(dot(normalize(vNormal), normalize(vec3(0.4, 0.8, 0.45))), 0.0);
        fragColour = vec4(min(c.rgb * shade * brightness, vec3(1.0)), 1.0);
        return;
    }
    vec4 fragPrimary = dynamicLight ? vec4(lightingStandIn, 1.0) : vec4(0.0);
    vec4 previous = vec4(0.0), bufferColour = vec4(0.0);
    for (int i = 0; i < stageCount; ++i) {
        if (((bufferBits >> i) & 1) != 0) bufferColour = previous;
        int src = stageWords[i].x, op = stageWords[i].y, comb = stageWords[i].z, scale = stageWords[i].w;
        vec4 s0 = source(src & 15, i, previous, bufferColour, fragPrimary);
        vec4 s1 = source((src >> 4) & 15, i, previous, bufferColour, fragPrimary);
        vec4 s2 = source((src >> 8) & 15, i, previous, bufferColour, fragPrimary);
        vec3 rgb = clamp(combine3(comb & 15, rgbOperand(s0, op & 15), rgbOperand(s1, (op >> 4) & 15), rgbOperand(s2, (op >> 8) & 15))
                         * float(1 << (scale & 3)), 0.0, 1.0);
        vec4 t0 = source((src >> 16) & 15, i, previous, bufferColour, fragPrimary);
        vec4 t1 = source((src >> 20) & 15, i, previous, bufferColour, fragPrimary);
        vec4 t2 = source((src >> 24) & 15, i, previous, bufferColour, fragPrimary);
        float a = clamp(combine1((comb >> 16) & 15, alphaOperand(t0, (op >> 12) & 7), alphaOperand(t1, (op >> 16) & 7),
                                 alphaOperand(t2, (op >> 20) & 7)) * float(1 << ((scale >> 16) & 3)), 0.0, 1.0);
        previous = vec4(rgb, a);
    }
    if (alphaReference >= 0.0 && !(previous.a > alphaReference)) discard;
    fragColour = vec4(min(previous.rgb * brightness, vec3(1.0)), previous.a);
}
