#pragma once

#include <array>
#include <cstddef>

namespace qol {

struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

inline bool operator==(const Rect& a, const Rect& b) {
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

constexpr int originalViewWidth = 400;
constexpr int originalViewHeight = 240;
constexpr int widestAspectWidth = 16;
constexpr int widestAspectHeight = 9;
constexpr int narrowestAspectWidth = 5;
constexpr int narrowestAspectHeight = 3;

struct ViewLayout {
    Rect view;
    std::array<Rect, 2> bars{};
    std::size_t barCount = 0;
    int aspectWidth = 0;
    int aspectHeight = 0;
};

enum class Corner { TopLeft, TopRight, BottomLeft };

struct HudAnchor {
    Corner corner = Corner::TopLeft;
    double x = 0;
    double y = 0;
    double scale = 1;
};

struct HudPoint {
    double x = 0;
    double y = 0;
};

ViewLayout computeViewLayout(int windowWidth, int windowHeight);

ViewLayout layoutInArea(int areaWidth, int areaHeight);

HudAnchor hudAnchor(const ViewLayout& layout, Corner corner);

HudPoint hudPosition(const HudAnchor& anchor, double originalX, double originalY);

}
