#pragma once

#include "qol/bottom_hook.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace qol {

inline constexpr float frontTopDim = 0.5f;
inline constexpr float pageWidth = 320;
inline constexpr float pageHeight = 240;

struct PageRect {
    float x, y, width, height, scale;
};

PageRect menuPageRect(int windowWidth, int windowHeight);
std::optional<std::array<float, 2>> pagePoint(const PageRect& page, float windowX, float windowY);

struct Box {
    float x, y, width, height;
};

bool contains(const Box& box, float x, float y);

struct FrontPanels {
    Box top;
    PageRect panel;
};

std::optional<FrontPanels> frontPanelsFor(const Settings& settings, int windowWidth, int windowHeight, int topWidth, int screenHeight);
std::optional<FrontPanels> frontPanels(int windowWidth, int windowHeight, int topWidth, int screenHeight);

void darken(std::uint8_t* rgba, std::size_t pixelCount, float factor);

enum class FrontButton { None, A, B, Y };

struct ClickTarget {
    Box box;
    bool isRow = false;
    std::size_t row = 0;
    FrontButton button = FrontButton::None;
};

struct ClickAction {
    std::optional<std::size_t> selectRow;
    FrontButton press = FrontButton::None;
    bool any() const { return selectRow || press != FrontButton::None; }
};

ClickAction classifyClick(const std::vector<ClickTarget>& targets, float x, float y);

}
