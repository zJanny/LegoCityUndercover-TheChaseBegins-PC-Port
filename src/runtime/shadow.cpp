#include "runtime/shadow.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cmath>

namespace chase::runtime {
namespace {
using Vec3 = std::array<float, 3>;

Vec3 row(const ShadowMatrix& m, int r) { return {m[std::size_t(r * 4)], m[std::size_t(r * 4 + 1)], m[std::size_t(r * 4 + 2)]}; }
Vec3 translation(const ShadowMatrix& m) { return {m[12], m[13], m[14]}; }
Vec3 add(const Vec3& a, const Vec3& b, float s) { return {a[0] + b[0] * s, a[1] + b[1] * s, a[2] + b[2] * s}; }
float length(const Vec3& v) { return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }

bool flag(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    return param && param->type == 5 && param->value != 0;
}
}

std::optional<ShadowLight> shadowLight(const std::vector<assets::LightObject>& lights, const std::optional<float>& levelRadius) {
    for (std::size_t i = 0; i < lights.size(); ++i) {
        const assets::LightObject& light = lights[i];
        if (!light.castShadows) continue;
        ShadowLight out;
        out.lightIndex = i;
        out.opacity = std::clamp(light.shadowOpacity.value_or(kShadowOpacityDefault), 0.0f, 1.0f);
        if (light.shadowColour) out.colour = *light.shadowColour;
        out.radius = light.type == 1 ? light.shadowRadius.value_or(kShadowRadiusDefault) : levelRadius.value_or(kShadowRadiusDefault);
        return out;
    }
    return std::nullopt;
}

std::array<std::uint8_t, 3> shadowConstant(const std::array<float, 3>& colour, float opacity) {
    std::array<std::uint8_t, 3> out{};
    for (std::size_t c = 0; c < 3; ++c) out[c] = std::uint8_t(int((255.0f - colour[c]) * opacity + 0.5f) & 0xff);
    return out;
}

std::array<float, 3> shadowFactor(const std::array<std::uint8_t, 3>& constant, float lit) {
    std::array<float, 3> out{};
    for (std::size_t c = 0; c < 3; ++c) {
        const float k = float(constant[c]) / 255.0f;
        out[c] = std::clamp(lit * k + (1.0f - k), 0.0f, 1.0f);
    }
    return out;
}

std::optional<std::array<float, 3>> shadowFocus(const ShadowMatrix& camera, float frustumY, float frustumZ, float playerY, float radius) {
    const Vec3 eye = translation(camera);
    if (!(radius > std::fabs(playerY - eye[1]))) return std::nullopt;
    const Vec3 ray = add(add({0, 0, 0}, row(camera, 1), frustumY), row(camera, 2), -frustumZ);
    if (!(ray[1] < 0.0f)) return std::nullopt;
    const float limit = radius * kShadowFocusAhead;
    float t = (playerY - eye[1]) / ray[1];
    if (t >= limit)
        t = limit;
    else if (t <= -limit)
        t = -limit;
    Vec3 point = add(eye, ray, t);
    Vec3 ahead = row(camera, 2);
    ahead[1] = 0.0f;
    const float size = length(ahead);
    if (size > 0.0f)
        for (float& v : ahead) v /= size;
    return add(point, ahead, limit);
}

ShadowCamera shadowCamera(const ShadowMatrix& light, const std::array<float, 3>& focus, const ShadowMatrix& camera, float radius) {
    ShadowCamera out;
    out.radius = radius;
    Vec3 centre = focus;
    const Vec3 eye = translation(camera);
    const float distance = length({focus[0] - eye[0], focus[1] - eye[1], focus[2] - eye[2]});
    const float limit = radius * kShadowFocusAhead;
    if (distance < limit) centre = add(focus, row(camera, 2), limit - distance);
    out.world = light;
    const Vec3 position = add(centre, row(light, 2), kShadowNear - 1.0f);
    out.world[12] = position[0];
    out.world[13] = position[1];
    out.world[14] = position[2];
    out.world[15] = 1.0f;
    return out;
}

std::array<float, 16> shadowViewProjection(const ShadowCamera& camera) {
    const Vec3 position = translation(camera.world);
    const float depthScale = 2.0f / (camera.farZ - camera.nearZ);
    const float depthOffset = -(camera.farZ + camera.nearZ) / (camera.farZ - camera.nearZ);
    const float scale[3] = {1.0f / camera.radius, 1.0f / camera.radius, depthScale};
    std::array<float, 16> out{};
    for (int r = 0; r < 3; ++r) {
        const Vec3 axis = row(camera.world, r);
        for (int c = 0; c < 3; ++c) out[std::size_t(c * 4 + r)] = axis[std::size_t(c)] * scale[r];
        out[std::size_t(12 + r)] = -(axis[0] * position[0] + axis[1] * position[1] + axis[2] * position[2]) * scale[r];
    }
    out[14] += depthOffset;
    out[15] = 1.0f;
    return out;
}

ShadowFlags shadowFlags(const assets::LevelObject& object, const assets::Level& cls) {
    return {flag(object, cls, "Shadow_Caster"), flag(object, cls, "Shadow_Receiver")};
}

}
