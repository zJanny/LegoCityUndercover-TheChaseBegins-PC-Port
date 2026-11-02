#pragma once
// Read-only description of what a front-end bottom page offers to a pointer: its rows and its button pictures in bottom-canvas
// pixels (320x240). The pages build it from the same layout elements and constants they draw with. No SDL dependency.
#include "frontend/flow.hpp"
#include "frontend/pages.hpp"
#include "frontend/resources.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace chase::frontend {

struct TargetBox {
    float x = 0, y = 0, width = 0, height = 0;
};

struct ButtonTarget {
    TargetBox box;
    std::uint32_t button = 0;
};

struct PageTargets {
    std::vector<TargetBox> rows;
    std::vector<ButtonTarget> buttons;
    bool empty() const { return rows.empty() && buttons.empty(); }
};

// The language list has no row pictures: its rows are the full-width bands of the text pitch drawn by FrontEnd::drawLanguage.
constexpr float kLanguageRowCentreY = 20.0f, kLanguageRowPitch = 24.0f;

PageTargets alertPageTargets(Resources& resources, const AlertPage& page);
PageTargets languageListTargets(Resources& resources, std::size_t rowCount);
// The rows slide in with the TransitionOn storyboard of the slot page, `seconds` after the page began.
PageTargets slotPageTargets(Resources& resources, std::size_t rowCount, bool selectedRowUsed, float seconds);

}
