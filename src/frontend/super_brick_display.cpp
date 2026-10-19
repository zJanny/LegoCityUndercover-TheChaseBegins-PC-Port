#include "frontend/super_brick_display.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace chase::frontend {

namespace {

constexpr const char* kModelPath = "models/pickup_debris_small/pickup_debris_small.bfnmdl";
constexpr const char* kTexturePath = "models/textures/pickup_debris.btga";
constexpr float kIconScale = 47.0f;        // fitted pixels per model unit
constexpr float kIconOffsetX = 0.0f;       // fitted
constexpr float kIconOffsetY = -7.0f;      // fitted
constexpr float kLight = 0.65f;            // fitted: one light factor for all faces
constexpr int kIconHalfSize = 20;
constexpr std::array<std::array<float, 2>, 3> kTextShadow = {{{1, 1}, {1, 0}, {0, 1}}};  // fitted to the oracle's one pixel shadow

float tintChannel(std::uint32_t abgr, int shift) {
    return float((abgr >> shift) & 0xff) / 255.0f;
}

}

void SuperBrickDisplayDraw::load() {
    loaded_ = true;
    const assets::Model model = assets::parseModel(resources_.bytes(kModelPath));
    if (model.meshes.empty() || model.meshes[0].parts.empty()) return;
    const assets::ModelMesh& mesh = model.meshes[0];
    centreY_ = mesh.centre[1];
    for (const assets::ModelPart& part : mesh.parts) {
        const unsigned base = unsigned(vertices_.size());
        for (const assets::Vertex& vertex : part.vertices)
            vertices_.push_back({vertex.position[0], vertex.position[1], vertex.position[2], vertex.uv0[0], vertex.uv0[1]});
        for (const std::uint16_t index : part.indices) indices_.push_back(base + index);
    }
}

void SuperBrickDisplayDraw::drawIcon(Canvas& top, float centreX, float centreY, float spin) {
    if (!loaded_) load();
    if (indices_.empty()) return;
    const assets::Image& texture = resources_.image(kTexturePath);
    const float cy = std::cos(-spin), sy = std::sin(-spin);
    const float ct = std::cos(runtime::kSuperBrickIconTilt), st = std::sin(runtime::kSuperBrickIconTilt);
    // Row vectors: 0x2354b4 premultiplies the matrix 0x237bc4 made, so a vertex is tilted first and then spun: v * Rx(tilt) * Ry(-spin).
    struct Projected {
        float x, y, z, u, v;
    };
    std::vector<Projected> projected;
    projected.reserve(vertices_.size());
    for (const Vertex& vertex : vertices_) {
        const float x = vertex.x, y = vertex.y - centreY_, z = vertex.z;
        const float y1 = y * ct - z * st, z1 = y * st + z * ct;
        const float x2 = x * cy + z1 * sy, z2 = -x * sy + z1 * cy;
        const float y2 = y1;
        projected.push_back({centreX + kIconOffsetX + x2 * kIconScale, centreY + kIconOffsetY - y2 * kIconScale, z2, vertex.u, vertex.v});
    }
    const int size = kIconHalfSize * 2 + 1;
    const int originX = int(std::floor(centreX)) - kIconHalfSize, originY = int(std::floor(centreY + kIconOffsetY)) - kIconHalfSize;
    std::vector<float> depth(std::size_t(size) * std::size_t(size), -1e9f);
    const std::uint32_t tint = runtime::kSuperBrickIconTint;
    const float tr = tintChannel(tint, 0), tg = tintChannel(tint, 8), tb = tintChannel(tint, 16);
    for (std::size_t i = 0; i + 2 < indices_.size(); i += 3) {
        const Projected& a = projected[indices_[i]];
        const Projected& b = projected[indices_[i + 1]];
        const Projected& c = projected[indices_[i + 2]];
        const float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        if (std::fabs(area) < 1e-6f) continue;
        const int minX = std::max(originX, int(std::floor(std::min({a.x, b.x, c.x})))), maxX = std::min(originX + size - 1, int(std::ceil(std::max({a.x, b.x, c.x}))));
        const int minY = std::max(originY, int(std::floor(std::min({a.y, b.y, c.y})))), maxY = std::min(originY + size - 1, int(std::ceil(std::max({a.y, b.y, c.y}))));
        for (int py = minY; py <= maxY; ++py)
            for (int px = minX; px <= maxX; ++px) {
                const float fx = float(px) + 0.5f, fy = float(py) + 0.5f;
                const float w0 = ((b.x - fx) * (c.y - fy) - (b.y - fy) * (c.x - fx)) / area;
                const float w1 = ((c.x - fx) * (a.y - fy) - (c.y - fy) * (a.x - fx)) / area;
                const float w2 = 1.0f - w0 - w1;
                if (w0 < 0 || w1 < 0 || w2 < 0) continue;
                const float z = w0 * a.z + w1 * b.z + w2 * c.z;
                float& stored = depth[std::size_t(py - originY) * std::size_t(size) + std::size_t(px - originX)];
                if (z <= stored) continue;
                stored = z;
                const float u = w0 * a.u + w1 * b.u + w2 * c.u, v = w0 * a.v + w1 * b.v + w2 * c.v;
                const int tx = std::clamp(int(u * float(texture.width)), 0, texture.width - 1);
                const int ty = std::clamp(int(v * float(texture.height)), 0, texture.height - 1);
                const std::uint8_t* texel = &texture.rgba[(std::size_t(ty) * std::size_t(texture.width) + std::size_t(tx)) * 4];
                if (px < 0 || py < 0 || px >= top.width() || py >= top.height()) continue;
                std::uint8_t* out = top.pixel(px, py);
                out[0] = std::uint8_t(std::clamp(float(texel[0]) * tr * kLight, 0.0f, 255.0f));
                out[1] = std::uint8_t(std::clamp(float(texel[1]) * tg * kLight, 0.0f, 255.0f));
                out[2] = std::uint8_t(std::clamp(float(texel[2]) * tb * kLight, 0.0f, 255.0f));
                out[3] = 255;
            }
    }
}

void SuperBrickDisplayDraw::draw(Canvas& top, const runtime::SuperBrickDisplayView& view) {
    if (view.iconVisible) drawIcon(top, view.iconX, view.iconY, view.spin);
    if (!view.textVisible) return;
    const BitmapFont& font = resources_.font("frontendfont");
    const auto& extended = resources_.extendedCharacters();
    const std::string text = std::to_string(view.count);
    const std::uint32_t colour = runtime::kSuperBrickTextColour;
    const Colour yellow{tintChannel(colour, 0), tintChannel(colour, 8), tintChannel(colour, 16), 1.0f};
    for (const auto& offset : kTextShadow)
        drawText(top, font, extended, text, view.textX + offset[0], view.textY + offset[1], Align::Left, {0, 0, 0, 1}, 0, false);
    drawText(top, font, extended, text, view.textX, view.textY, Align::Left, yellow, 0, false);
}

}
