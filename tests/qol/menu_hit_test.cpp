// Scale rule, hit areas and event mapping of the settings page, pinned to qol-settings-drawing.md §2026-10-29T12:11:55 (section 2 examples,
// section 4 navigation) and the user's answers of 2026-10-29 on the tap circle and hover. The geometry values are those menuPageSetup reads
// from the round menu and shortcut layouts (20261029-qolmenu2-A, 2026-10-29, chase-layoutdump of sprites/ui_roundaboutmenu/main.bxaml and
// sprites/blend/ui_frontend/shortcuts.bxaml; slot centres as pause-menu-real-data pins them).
#include "qol/menu_hit.hpp"

#include <cmath>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    ++failures;
    std::cerr << "FAIL: " << what << "\n";
}

bool near(float a, float b) {
    return std::fabs(a - b) < 1e-4f;
}

bool same(const std::optional<qol::Event>& event, qol::EventKind kind, std::size_t index = 0) {
    return event && event->kind == kind && event->index == index;
}

qol::MenuGeometry geometry() {
    qol::MenuGeometry g;
    g.slotCentres = {{{54, 119}, {159, 119}, {264, 119}}};
    g.slotRadius = 32;
    g.back = {11, 209, 32, 32};
    g.select = {276, 209, 32, 32};
    g.previousTab = {2, 95, 16, 32};
    g.nextTab = {298, 95, 16, 32};
    g.header = {-70, 31.667f, 256, 32};
    return g;
}

void scaleRule() {
    const qol::PageRect small = qol::menuPageRect(400, 240);
    require(near(small.scale, 1) && near(small.x, 40) && near(small.y, 0) && near(small.width, 320) && near(small.height, 240),
        "400x240: s = 1 with 40-pixel bars left and right");
    const qol::PageRect full = qol::menuPageRect(1920, 1080);
    require(near(full.scale, 4.5f) && near(full.width, 1440) && near(full.height, 1080) && near(full.x, 240) && near(full.y, 0),
        "1920x1080: s = 4.5, a 1440x1080 page and 240-pixel bars");
    const qol::PageRect wide = qol::menuPageRect(3440, 1440);
    require(near(wide.scale, 6) && near(wide.width, 1920) && near(wide.height, 1440) && near(wide.x, 760),
        "3440x1440: s = 6, a 1920x1440 page");
    const qol::PageRect tall = qol::menuPageRect(640, 960);
    require(near(tall.scale, 2) && near(tall.x, 0) && near(tall.y, 240), "a tall window gets bars above and below");

    const auto inside = qol::pagePoint(full, 240 + 4.5f * 159, 4.5f * 119);
    require(inside && near((*inside)[0], 159) && near((*inside)[1], 119), "window pixel to page pixel");
    require(!qol::pagePoint(full, 100, 500), "a pixel in the black bar is outside the page");
    require(!qol::pagePoint(full, 240 + 1440, 10), "the right edge is outside the page");
}

void roundAreas() {
    qol::Settings settings;
    qol::MenuModel model(settings);
    model.open();
    const std::vector<qol::HitArea> areas = qol::roundHitAreas(model, geometry());
    require(areas.size() == 8, "three slots and five frame areas");

    require(same(qol::MenuPointer{}.click(areas, 159, 119), qol::EventKind::RowClick, 0), "the centre slot clicks the selected row");
    require(same(qol::MenuPointer{}.click(areas, 54, 119), qol::EventKind::RowClick, 3), "the left slot shows the previous row with wrap");
    require(same(qol::MenuPointer{}.click(areas, 264, 119), qol::EventKind::RowClick, 1), "the right slot shows the next row");
    require(!qol::MenuPointer{}.click(areas, 159 + 33, 119), "the tap circle has radius 32");
    require(!qol::MenuPointer{}.click(areas, 159 + 23, 119 + 23), "outside the circle at the box corner");
    require(same(qol::MenuPointer{}.click(areas, 159 + 22, 119 + 22), qol::EventKind::RowClick, 0), "inside the circle");

    require(same(qol::MenuPointer{}.click(areas, 20, 220), qol::EventKind::Back), "the Back icon sends Back");
    require(same(qol::MenuPointer{}.click(areas, 290, 220), qol::EventKind::Activate), "the Select icon sends Activate");
    require(same(qol::MenuPointer{}.click(areas, 5, 100), qol::EventKind::TabPrevious), "the left arrow sends TabPrevious");
    require(same(qol::MenuPointer{}.click(areas, 305, 100), qol::EventKind::TabNext), "the right arrow sends TabNext");
    require(!qol::MenuPointer{}.click(areas, 100, 40), "a click on the tab header sends nothing");

    model.handle(qol::rowMove(2));
    const std::vector<qol::HitArea> moved = qol::roundHitAreas(model, geometry());
    require(same(qol::MenuPointer{}.click(moved, 54, 119), qol::EventKind::RowClick, 1), "the round follows the selection");

    model.handle({qol::EventKind::TabNext});
    const std::vector<qol::HitArea> display = qol::roundHitAreas(model, geometry());
    require(same(qol::MenuPointer{}.click(display, 54, 119), qol::EventKind::RowClick, 1) &&
            same(qol::MenuPointer{}.click(display, 264, 119), qol::EventKind::RowClick, 1),
        "with two rows both neighbours show the other row");
    qol::MenuPointer header;
    require(same(header.move(display, 100, 40), qol::EventKind::TabMove, 1), "hover over the header sends tabMove of the shown tab");
}

void hoverOnEnter() {
    qol::Settings settings;
    qol::MenuModel model(settings);
    model.open();
    qol::MenuPointer pointer;
    std::vector<qol::HitArea> areas = qol::roundHitAreas(model, geometry());
    require(!pointer.move(areas, 100, 180), "no event outside every area");
    const auto entered = pointer.move(areas, 54, 119);
    require(same(entered, qol::EventKind::RowMove, 3), "entering the left slot moves to its row");
    model.handle(*entered);
    areas = qol::roundHitAreas(model, geometry());
    require(!pointer.move(areas, 55, 120), "a pointer resting in the same slot after the turn sends nothing");
    require(!pointer.move(areas, 20, 220), "the Back icon has no hover event");
    require(same(pointer.move(areas, 54, 119), qol::EventKind::RowMove, 2), "entering the slot again moves on");
    pointer.reset();
    require(same(pointer.move(areas, 54, 119), qol::EventKind::RowMove, 2), "after a reset the slot counts as entered");
}

void keysAndWheel() {
    require(same(qol::eventForWheel(1), qol::EventKind::Up) && same(qol::eventForWheel(3), qol::EventKind::Up), "wheel up moves up");
    require(same(qol::eventForWheel(-1), qol::EventKind::Down), "wheel down moves down");
    require(!qol::eventForWheel(0), "no wheel step, no event");
    const std::pair<qol::Key, qol::EventKind> keys[] = {
        {qol::Key::Up, qol::EventKind::Up}, {qol::Key::PadUp, qol::EventKind::Up},
        {qol::Key::Down, qol::EventKind::Down}, {qol::Key::PadDown, qol::EventKind::Down},
        {qol::Key::Left, qol::EventKind::Left}, {qol::Key::PadLeft, qol::EventKind::Left},
        {qol::Key::Right, qol::EventKind::Right}, {qol::Key::PadRight, qol::EventKind::Right},
        {qol::Key::Enter, qol::EventKind::Activate}, {qol::Key::PadA, qol::EventKind::Activate},
        {qol::Key::Escape, qol::EventKind::Back}, {qol::Key::PadB, qol::EventKind::Back},
        {qol::Key::PageUp, qol::EventKind::TabPrevious}, {qol::Key::PadL, qol::EventKind::TabPrevious},
        {qol::Key::PageDown, qol::EventKind::TabNext}, {qol::Key::PadR, qol::EventKind::TabNext}};
    for (const auto& [key, kind] : keys)
        require(qol::eventForKey(key).kind == kind, "key " + std::to_string(int(key)) + " maps as section 4 says");
}

void slotRows() {
    require(qol::roundSlotRow(0, 4, 0) == 3 && qol::roundSlotRow(0, 4, 1) == 0 && qol::roundSlotRow(0, 4, 2) == 1, "four rows");
    require(qol::roundSlotRow(3, 4, 2) == 0, "the next row wraps");
    require(qol::roundSlotRow(0, 1, 0) == 0 && qol::roundSlotRow(0, 1, 2) == 0, "one row fills every slot");
}

}

int main() {
    scaleRule();
    roundAreas();
    hoverOnEnter();
    keysAndWheel();
    slotRows();
    if (failures) return 1;
    std::cout << "qol-menu-hit: ok\n";
    return 0;
}
