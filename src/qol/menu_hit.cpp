#include "qol/menu_hit.hpp"

#include <algorithm>

namespace qol {

PageRect menuPageRect(int windowWidth, int windowHeight) {
    const float scale = std::min(float(windowWidth) / pageWidth, float(windowHeight) / pageHeight);
    const float width = pageWidth * scale, height = pageHeight * scale;
    return {(float(windowWidth) - width) / 2, (float(windowHeight) - height) / 2, width, height, scale};
}

std::optional<std::array<float, 2>> pagePoint(const PageRect& page, float windowX, float windowY) {
    if (page.scale <= 0) return std::nullopt;
    const float x = (windowX - page.x) / page.scale, y = (windowY - page.y) / page.scale;
    if (x < 0 || y < 0 || x >= pageWidth || y >= pageHeight) return std::nullopt;
    return std::array<float, 2>{x, y};
}

bool contains(const HitArea& area, float x, float y) {
    const Box& box = area.box;
    if (box.width <= 0 || box.height <= 0) return false;
    if (!area.round) return x >= box.x && y >= box.y && x < box.x + box.width && y < box.y + box.height;
    const float dx = (x - box.x - box.width / 2) / (box.width / 2), dy = (y - box.y - box.height / 2) / (box.height / 2);
    return dx * dx + dy * dy <= 1;
}

std::optional<std::size_t> areaAt(const std::vector<HitArea>& areas, float x, float y) {
    for (std::size_t i = 0; i < areas.size(); ++i)
        if (contains(areas[i], x, y)) return i;
    return std::nullopt;
}

std::size_t roundSlotRow(std::size_t selected, std::size_t count, std::size_t slot) {
    if (count == 0) return 0;
    return (selected % count + slot + count - 1) % count;
}

std::vector<HitArea> frameHitAreas(const MenuModel& model, const MenuGeometry& geometry) {
    std::vector<HitArea> areas;
    areas.push_back({geometry.back, false, Event{EventKind::Back}, std::nullopt});
    areas.push_back({geometry.select, false, Event{EventKind::Activate}, std::nullopt});
    areas.push_back({geometry.previousTab, false, Event{EventKind::TabPrevious}, std::nullopt});
    areas.push_back({geometry.nextTab, false, Event{EventKind::TabNext}, std::nullopt});
    areas.push_back({geometry.header, false, std::nullopt, tabMove(static_cast<std::size_t>(model.tab()))});
    return areas;
}

std::vector<HitArea> roundHitAreas(const MenuModel& model, const MenuGeometry& geometry) {
    std::vector<HitArea> areas;
    const std::size_t count = model.rows().size();
    if (count > 0) {
        for (std::size_t slot = 0; slot < roundVisibleSlots; ++slot) {
            const std::size_t row = roundSlotRow(model.row(), count, slot);
            const std::array<float, 2>& centre = geometry.slotCentres[slot];
            const float radius = geometry.slotRadius;
            areas.push_back({Box{centre[0] - radius, centre[1] - radius, 2 * radius, 2 * radius}, true, rowClick(row), rowMove(row)});
        }
    }
    for (const HitArea& area : frameHitAreas(model, geometry)) areas.push_back(area);
    return areas;
}

std::optional<Event> MenuPointer::move(const std::vector<HitArea>& areas, float x, float y) {
    const std::optional<std::size_t> area = areaAt(areas, x, y);
    if (area == over_) return std::nullopt;
    over_ = area;
    if (!area) return std::nullopt;
    return areas[*area].hover;
}

std::optional<Event> MenuPointer::click(const std::vector<HitArea>& areas, float x, float y) const {
    const std::optional<std::size_t> area = areaAt(areas, x, y);
    if (!area) return std::nullopt;
    return areas[*area].click;
}

std::optional<Event> eventForWheel(int steps) {
    if (steps > 0) return Event{EventKind::Up};
    if (steps < 0) return Event{EventKind::Down};
    return std::nullopt;
}

}
