#pragma once
// The parts of the in-game HUD that Hud::draw (hud.hpp) draws, as a set, so a caller can draw some of them into a canvas.
// The parts follow the draw code: the page layout's elements (docs/re/hud.md), the stud bar's number and icon (0x22ddf4), the super
// brick count display and the camera lock icon (0x1b1150).
#include <cstdint>
#include <set>
#include <string>

namespace chase::frontend {

enum class HudPart : std::uint8_t { Portrait, Hearts, Studs, SuperBricks, CameraLock };

constexpr int kHudPartCount = 5;

class HudPartSet {
public:
    constexpr HudPartSet() = default;
    constexpr HudPartSet(HudPart part) : bits_(std::uint8_t(1u << unsigned(part))) {}
    static constexpr HudPartSet all() {
        HudPartSet set;
        set.bits_ = std::uint8_t((1u << kHudPartCount) - 1);
        return set;
    }
    constexpr bool contains(HudPart part) const { return (bits_ >> unsigned(part)) & 1u; }
    constexpr bool empty() const { return bits_ == 0; }
    constexpr HudPartSet operator|(HudPartSet other) const {
        HudPartSet set;
        set.bits_ = std::uint8_t(bits_ | other.bits_);
        return set;
    }
    constexpr bool operator==(HudPartSet other) const { return bits_ == other.bits_; }

private:
    std::uint8_t bits_ = 0;
};

// The children of the page layout's root that belong to a part. The remaining children are hidden by the draw in every case.
const std::set<std::string>& hudPartLayoutElements(HudPart part);

// The layout elements to hide so that only the parts in `parts` are drawn from the page layout.
std::set<std::string> hudHiddenLayoutElements(HudPartSet parts);

}
