#include "runtime/zipline_rope.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {

using Vec = std::array<float, 3>;

Vec vec(const float v[3]) { return {v[0], v[1], v[2]}; }
Vec sub(const Vec& a, const Vec& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
float dot(const Vec& a, const Vec& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
Vec cross(const Vec& a, const Vec& b) { return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; }

Vec scaled(const Vec& a, float s) { return {a[0] * s, a[1] * s, a[2] * s}; }

// 0x23b998: the unit vector, (0, 0, 1) for a vector shorter than 2^-23.
Vec unitOrForward(const Vec& a) {
    const float length = std::sqrt(dot(a, a));
    std::uint32_t bits;
    std::memcpy(&bits, &length, 4);
    if (bits < 0x34000001u) return {0.0f, 0.0f, 1.0f};
    return scaled(a, 1.0f / length);
}

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

const assets::LevelParam* findParam(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    return effectiveParam(object, cls, assets::nameHash(name));
}

}

std::vector<ZiplineRope> loadZiplineRopes(const assets::Level& level, const assets::ClassLookup& classOf, const std::vector<ZiplineAnchor>& anchors) {
    std::vector<ZiplineRope> ropes(anchors.size());
    for (std::size_t i = 0; i < anchors.size(); ++i) {
        const assets::LevelObject& object = level.objects[anchors[i].order];
        const assets::Level* cls = classOf(object.className);
        if (!cls) continue;
        ZiplineRope& rope = ropes[i];
        if (const assets::LevelParam* texture = findParam(object, *cls, "RopeTexture"); texture && texture->type == 1 && !texture->blocks.empty()) {
            const assets::Bytes& block = texture->blocks.front();
            rope.texture = lower(std::string(block.begin(), std::find(block.begin(), block.end(), std::uint8_t(0))));
        }
        if (const assets::LevelParam* width = findParam(object, *cls, "RenderWidth")) {
            if (width->type == 0) std::memcpy(&rope.width, &width->value, 4);
            else rope.width = float(width->value);
        }
        const assets::LevelParam* fade = findParam(object, *cls, "FadeNearCamera");
        rope.fadeNearCamera = fade && fade->value != 0;
    }
    return ropes;
}

int ziplineRopeAlpha(const ZiplineAnchor& anchor, const ZiplineAnchor& target, const float camera[3]) {
    const Vec direction = unitOrForward(sub(vec(target.position), vec(anchor.position)));
    Vec side;
    if (std::fabs(direction[1]) < 0.98f) side = {direction[2], 0.0f, -direction[0]};
    else side = {0.0f, -direction[2], direction[1]};
    side = unitOrForward(side);
    const Vec up = cross(direction, side);
    const Vec offset = sub(vec(camera), vec(anchor.position));
    const float x = dot(offset, side), y = dot(offset, up);
    const float distance = std::sqrt(x * x + y * y);
    if (distance >= kRopeFadeEnd) return 255;
    if (distance <= kRopeFadeStart) return -1;
    return int((distance - kRopeFadeStart) * kRopeFadeScale * 255.0f) & 0xff;
}

std::array<RopeVertex, 4> ziplineRopeStrip(const float start[3], const float end[3], float width, const float viewDirection[3], std::uint8_t alpha) {
    const Vec p1 = vec(start), p2 = vec(end);
    const Vec side = unitOrForward(cross(sub(p2, p1), unitOrForward(vec(viewDirection))));
    const Vec midpoint = scaled({p1[0] + p2[0], p1[1] + p2[1], p1[2] + p2[2]}, 0.5f);
    const Vec a = sub(p1, midpoint), b = sub(p2, midpoint);
    const float dx = std::fabs(b[0] - a[0]), dy = std::fabs(b[1] - a[1]) * kRopeTextureRepeatY, dz = std::fabs(b[2] - a[2]);
    const float length = std::max({dx, dy, dz});
    std::array<RopeVertex, 4> strip{};
    const Vec ends[2] = {a, b};
    const float v[2] = {0.0f, length};
    for (int i = 0; i < 4; ++i) {
        const float sign = (i & 1) ? -width : width;
        RopeVertex& vertex = strip[std::size_t(i)];
        for (int axis = 0; axis < 3; ++axis) vertex.position[axis] = (ends[i >> 1][axis] + side[axis] * sign) + midpoint[axis];
        vertex.uv[0] = (i & 1) ? 0.0f : 1.0f;
        vertex.uv[1] = v[i >> 1];
        vertex.rgba[0] = vertex.rgba[1] = vertex.rgba[2] = 255;
        vertex.rgba[3] = alpha;
    }
    return strip;
}

std::vector<RopeDraw> ziplineRopeDraws(const std::vector<ZiplineAnchor>& anchors, const std::vector<ZiplineRope>& ropes, const float camera[3],
                                       const float viewDirection[3], const std::function<bool(const ZiplineAnchor&)>& enabled) {
    std::vector<RopeDraw> draws;
    for (std::size_t i = 0; i < anchors.size() && i < ropes.size(); ++i) {
        const ZiplineAnchor& anchor = anchors[i];
        const ZiplineRope& rope = ropes[i];
        if (rope.texture.empty() || anchor.target < 0) continue;
        const ZiplineAnchor& target = anchors[std::size_t(anchor.target)];
        if (anchor.order < target.order && (!enabled || enabled(target))) continue;
        if (enabled && !enabled(anchor)) continue;
        int alpha = 255;
        if (rope.fadeNearCamera) alpha = ziplineRopeAlpha(anchor, target, camera);
        if (alpha <= 0) continue;
        RopeDraw draw;
        draw.texture = rope.texture;
        draw.alpha = std::uint8_t(alpha);
        draw.strip = ziplineRopeStrip(anchor.position, target.position, rope.width, viewDirection, draw.alpha);
        draws.push_back(std::move(draw));
    }
    return draws;
}

}
