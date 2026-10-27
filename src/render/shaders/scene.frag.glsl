#version 460 core
in vec2 vUv; in vec2 vUv1; in vec3 vNormal; in vec4 vColour; in float vViewDepth; in vec3 vWorldNormal; in vec3 vWorldPosition; in vec3 vCube;
uniform sampler2D unit0; uniform sampler2D unit1; uniform sampler2D unit2; uniform samplerCube cube0;
uniform int stageCount; uniform ivec4 stageWords[6]; uniform vec4 stageConstant[6];
uniform int bufferBits; uniform int unitMode[3]; uniform int unitUv[3];
uniform bool hasNormal; uniform float alphaReference; uniform float brightness;
uniform bool lightingOn; uniform vec3 globalAmbient; uniform int lightCount; uniform vec3 eyePosition; uniform bool clampHighlights;
uniform vec3 lightDiffuse[8]; uniform vec3 lightAmbient[8]; uniform vec3 lightSpecular[8]; uniform vec3 lightVector[8];
uniform int lightFlags[8]; uniform float lightDistanceScale[8];
uniform uvec4 lutDistribution[64]; uniform uvec4 lutSpot[64]; uniform uvec4 lutDistance[64];
uniform float instanceFade; uniform vec3 instanceTint;
uniform bool fogOn; uniform vec3 fogColour; uniform vec2 fogProjection; uniform uint fogLut[128];
out vec4 fragColour;

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

uint lutWord(int table, int index) {
    uvec4 words = table == 0 ? lutDistribution[index >> 2] : table == 1 ? lutSpot[index >> 2] : lutDistance[index >> 2];
    return words[index & 3];
}
float lutValue(uint word, float delta) {
    int difference = int((word >> 12) & 0xfffu);
    if (difference >= 2048) difference -= 4096;
    return float(word & 0xfffu) / 4096.0 + float(difference) / 2048.0 * delta;
}
float lutUnsigned(int table, float x) {
    int index = int(clamp(floor(x * 256.0), 0.0, 255.0));
    return lutValue(lutWord(table, index), x * 256.0 - float(index));
}
float lutSigned(int table, float x) {
    int index = int(clamp(floor(x * 128.0), -128.0, 127.0));
    return lutValue(lutWord(table, index & 255), x * 128.0 - float(index));
}
void fragmentLighting(out vec4 primary, out vec4 secondary) {
    primary = vec4(0.0);
    secondary = vec4(0.0);
    if (!lightingOn) return;
    vec3 n = dot(vWorldNormal, vWorldNormal) > 0.0 ? normalize(vWorldNormal) : vec3(0.0, 1.0, 0.0);
    vec3 v = normalize(eyePosition - vWorldPosition);
    vec3 diffuseSum = vec3(0.0), specularSum = vec3(0.0);
    for (int i = 0; i < lightCount; ++i) {
        int flags = lightFlags[i];
        bool directional = (flags & 1) != 0, twoSided = (flags & 2) != 0;
        vec3 toLight = directional ? lightVector[i] : lightVector[i] - vWorldPosition;
        vec3 l = normalize(toLight);
        float ln = dot(l, n);
        float diffuseDot = twoSided ? abs(ln) : max(ln, 0.0);
        float nh = dot(n, normalize(l + v));
        float distribution = lutUnsigned(0, twoSided ? abs(nh) : max(nh, 0.0));
        float spot = (flags & 4) != 0 ? lutSigned(1, ln) : 1.0;
        float distance = (flags & 8) != 0 ? lutUnsigned(2, clamp(length(toLight) * lightDistanceScale[i], 0.0, 1.0)) : 1.0;
        float highlight = clampHighlights ? (ln <= 0.0 ? 0.0 : 1.0) : 1.0;
        diffuseSum += (lightDiffuse[i] * diffuseDot + lightAmbient[i]) * distance * spot;
        specularSum += lightSpecular[i] * distribution * highlight * distance * spot;
    }
    primary = vec4(clamp(diffuseSum + globalAmbient, 0.0, 1.0), 1.0);
    secondary = vec4(clamp(specularSum, 0.0, 1.0), 1.0);
}

vec4 unitColour(int unit) {
    if (unitMode[unit] == 0) return vec4(1.0);
    if (unitMode[unit] == 2) return texture(cube0, vCube);
    if (unitMode[unit] == 3) return vec4(0.0);
    vec2 uv = unitUv[unit] == 1 ? vUv1 : vUv;
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
vec4 source(int s, int stage, vec4 previous, vec4 bufferColour, vec4 fragPrimary, vec4 fragSecondary) {
    if (s == 0) return vColour;
    if (s == 1) return fragPrimary;
    if (s == 2) return fragSecondary;
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
        fragColour = vec4(applyFog(min(c.rgb * instanceTint * shade * brightness, vec3(1.0))) * instanceFade, instanceFade);
        return;
    }
    vec4 fragPrimary, fragSecondary;
    fragmentLighting(fragPrimary, fragSecondary);
    vec4 previous = vec4(0.0), bufferColour = vec4(0.0);
    for (int i = 0; i < stageCount; ++i) {
        if (((bufferBits >> i) & 1) != 0) bufferColour = previous;
        int src = stageWords[i].x, op = stageWords[i].y, comb = stageWords[i].z, scale = stageWords[i].w;
        vec4 s0 = source(src & 15, i, previous, bufferColour, fragPrimary, fragSecondary);
        vec4 s1 = source((src >> 4) & 15, i, previous, bufferColour, fragPrimary, fragSecondary);
        vec4 s2 = source((src >> 8) & 15, i, previous, bufferColour, fragPrimary, fragSecondary);
        vec3 rgb = clamp(combine3(comb & 15, rgbOperand(s0, op & 15), rgbOperand(s1, (op >> 4) & 15), rgbOperand(s2, (op >> 8) & 15))
                         * float(1 << (scale & 3)), 0.0, 1.0);
        vec4 t0 = source((src >> 16) & 15, i, previous, bufferColour, fragPrimary, fragSecondary);
        vec4 t1 = source((src >> 20) & 15, i, previous, bufferColour, fragPrimary, fragSecondary);
        vec4 t2 = source((src >> 24) & 15, i, previous, bufferColour, fragPrimary, fragSecondary);
        float a = clamp(combine1((comb >> 16) & 15, alphaOperand(t0, (op >> 12) & 7), alphaOperand(t1, (op >> 16) & 7),
                                 alphaOperand(t2, (op >> 20) & 7)) * float(1 << ((scale >> 16) & 3)), 0.0, 1.0);
        previous = vec4(rgb, a);
    }
    if (alphaReference >= 0.0 && !(previous.a > alphaReference)) discard;
    fragColour = vec4(applyFog(min(previous.rgb * instanceTint * brightness, vec3(1.0))) * instanceFade, previous.a * instanceFade);
}
