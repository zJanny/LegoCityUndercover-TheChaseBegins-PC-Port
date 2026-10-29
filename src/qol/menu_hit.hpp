#pragma once

#include "qol/menu_model.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

namespace qol {

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

struct HitArea {
    Box box;
    bool round = false;
    std::optional<Event> click;
    std::optional<Event> hover;
};

bool contains(const HitArea& area, float x, float y);
std::optional<std::size_t> areaAt(const std::vector<HitArea>& areas, float x, float y);

struct MenuGeometry {
    std::array<std::array<float, 2>, 3> slotCentres{};
    float slotRadius = 0;
    Box back{}, select{}, previousTab{}, nextTab{}, header{};
};

inline constexpr std::size_t roundVisibleSlots = 3;

std::size_t roundSlotRow(std::size_t selected, std::size_t count, std::size_t slot);
std::vector<HitArea> frameHitAreas(const MenuModel& model, const MenuGeometry& geometry);
std::vector<HitArea> roundHitAreas(const MenuModel& model, const MenuGeometry& geometry);

class MenuPointer {
public:
    std::optional<Event> move(const std::vector<HitArea>& areas, float x, float y);
    std::optional<Event> click(const std::vector<HitArea>& areas, float x, float y) const;
    void reset() { over_.reset(); }

private:
    std::optional<std::size_t> over_;
};

std::optional<Event> eventForWheel(int steps);

}
