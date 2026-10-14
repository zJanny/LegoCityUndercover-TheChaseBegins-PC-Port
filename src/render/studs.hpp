#pragma once
// Stud sprites of chase-game: camera-facing quads that show one frame of the 4x4 sheet
// sprites/commonlego/pickups_commonstud, tinted per kind, as the pickup draw 0x17fc38 / 0x1d1bb8 builds them
// (docs/re/studs.md). The blend state of the original is not replicated: alpha-tested like the other models.
#include "assets/assets.hpp"
#include "render/scene.hpp"
#include <cstdint>
#include <memory>
#include <vector>

namespace chase::render {

struct StudSprite {
    float centre[3];
    std::uint32_t tint;   // 0xAABBGGRR
};

class StudSpritesImpl;

class StudSprites {
public:
    explicit StudSprites(assets::FuseArchive& archive);
    ~StudSprites();
    StudSprites(const StudSprites&) = delete;
    StudSprites& operator=(const StudSprites&) = delete;

    // `frame` 0..15 selects the cell of the sheet (row-major, row 0 on top); `halfSize` is the quad's half width.
    void draw(const Mat4& view, const Mat4& viewProjection, const std::vector<StudSprite>& sprites, unsigned frame, float halfSize) const;

private:
    std::unique_ptr<StudSpritesImpl> impl_;
};

}
