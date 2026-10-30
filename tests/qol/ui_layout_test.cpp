// Spec: docs/design/qol-settings-drawing.md entry 2026-10-30T21:56:04 (text 2.2 to 2.8 percent of the window height,
// rows 5 to 6 percent, hit areas at least 40 px at 1080p, contrast at least 4.5:1, selected tab underlined,
// focus by bar plus border, tabs fixed while long tabs scroll).
// Pinned sizes computed by hand from the chosen fractions (text 2.5 percent, row 5.5 percent) by 20261030-qolui-C on 2026-10-30.
// Text width is a fixed-width fake: 0.5 * pixel size per character.
#include "qol/ui_layout.hpp"

#include <cmath>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

float fakeWidth(std::string_view text, float pixelSize) { return 0.5f * pixelSize * static_cast<float>(text.size()); }

bool inside(const qol::Rect& inner, const qol::Rect& outer) {
    return inner.x >= outer.x && inner.y >= outer.y && inner.x + inner.width <= outer.x + outer.width &&
           inner.y + inner.height <= outer.y + outer.height;
}

bool overlap(const qol::Rect& a, const qol::Rect& b) {
    return a.x < b.x + b.width && b.x < a.x + a.width && a.y < b.y + b.height && b.y < a.y + a.height;
}

qol::ui::Screen sampleScreen(int w, int h, std::size_t rowCount) {
    using namespace qol::ui;
    Screen s;
    s.windowWidth = w;
    s.windowHeight = h;
    s.tabs = {"Game", "Display", "Controls", "Quality of life"};
    const RowKind kinds[] = {RowKind::Slider, RowKind::Selector, RowKind::Toggle, RowKind::Dropdown, RowKind::Button};
    for (std::size_t i = 0; i < rowCount; ++i)
        s.rows.push_back({"Setting " + std::to_string(i), kinds[i % 5], "On", 0.5, true});
    return s;
}

void checkScale(int w, int h, float text, int row, int minHit) {
    const qol::ui::Scale s = qol::ui::computeScale(w, h);
    const std::string name = std::to_string(h) + "p ";
    require(std::fabs(s.textPixels - text) < 0.01f, name + "text pixels");
    require(s.textPixels >= 0.022f * static_cast<float>(h) && s.textPixels <= 0.028f * static_cast<float>(h), name + "text within 2.2 to 2.8 percent");
    require(s.rowHeight == row, name + "row height");
    require(s.rowHeight >= static_cast<int>(std::floor(0.05 * h)) && s.rowHeight <= static_cast<int>(std::ceil(0.06 * h)), name + "row within 5 to 6 percent");
    require(s.minHit == minHit, name + "min hit");
    require(s.rowHeight >= s.minHit, name + "row at least min hit");
}

void checkLayout(int w, int h) {
    using namespace qol::ui;
    const std::string name = std::to_string(h) + "p ";
    Screen screen = sampleScreen(w, h, 5);
    screen.focusRow = 2;
    screen.selectedTab = 1;
    const UiLayout layout = computeLayout(screen, fakeWidth);
    const Scale& s = layout.scale;

    require(layout.tabs.size() == 4, name + "four tabs");
    int left = layout.tabs.front().rect.x;
    int right = layout.tabs.back().rect.x + layout.tabs.back().rect.width;
    require(std::abs(left - (w - right)) <= 1, name + "tabs centred");
    for (std::size_t i = 0; i < layout.tabs.size(); ++i) {
        require(layout.tabs[i].rect.height >= s.minHit && layout.tabs[i].rect.width >= s.minHit, name + "tab hit size");
        require(layout.tabs[i].selected == (i == 1), name + "tab selected flag");
        require((layout.tabs[i].underline.width > 0) == (i == 1), name + "only selected tab underlined");
        require(layout.tabs[i].labelFits, name + "tab label fits");
        if (i > 0) require(layout.tabs[i].rect.x == layout.tabs[i - 1].rect.x + layout.tabs[i - 1].rect.width, name + "tabs adjacent");
    }
    require(layout.tabs[1].underline.height >= 2, name + "underline thick");

    require(std::abs(layout.viewport.x - (w - layout.viewport.x - layout.viewport.width)) <= 1, name + "column centred");
    require(layout.viewport.y >= s.tabBarHeight, name + "column below tab bar");
    require(layout.viewport.y + layout.viewport.height <= layout.helpLine.y, name + "column above help line");
    require(layout.helpLine.y + layout.helpLine.height == layout.hintBar.y, name + "help line above hint bar");
    require(layout.hintBar.y + layout.hintBar.height == h, name + "hint bar at bottom");
    require(layout.backButton.x < w / 4 && layout.backButton.y == layout.hintBar.y, name + "back button bottom left");
    require(layout.backButton.height >= s.minHit && layout.backButton.width >= s.minHit, name + "back hit size");
    require(!overlap(layout.backButton, layout.hintText), name + "back and hint text apart");

    for (std::size_t i = 0; i < layout.rows.size(); ++i) {
        const RowLayout& row = layout.rows[i];
        require(row.visible, name + "row visible");
        require(row.rect.height == s.rowHeight, name + "row height in layout");
        require(row.rect.height >= s.minHit, name + "row hit height");
        require(inside(row.label, row.rect) && inside(row.control, row.rect), name + "label and control in row");
        require(row.label.x + row.label.width <= row.control.x, name + "label left of control");
        require(row.control.x + row.control.width <= row.rect.x + row.rect.width, name + "control right");
        require(row.labelFits, name + "label fits");
        require((i == 2) == row.focused, name + "focus flag");
        require((i == 2) == (row.focusBar.width > 0 && row.focusBorder.width > 0), name + "focus bar plus border together");
        if (i > 0) require(row.rect.y == layout.rows[i - 1].rect.y + s.rowHeight, name + "rows stacked");
    }

    const RowLayout& slider = layout.rows[0];
    require(inside(slider.track, slider.control) && inside(slider.thumb, slider.control) && inside(slider.numberBox, slider.control), name + "slider parts in control");
    require(!overlap(slider.track, slider.numberBox), name + "slider track and number apart");
    require(slider.thumb.width >= 1 && slider.numberBox.width >= static_cast<int>(fakeWidth("100", s.textPixels)), name + "slider number fits 100");
    const RowLayout& selector = layout.rows[1];
    require(selector.leftArrow.width >= s.minHit && selector.rightArrow.width >= s.minHit, name + "arrow hit width");
    require(selector.leftArrow.height >= s.minHit, name + "arrow hit height");
    require(!overlap(selector.leftArrow, selector.valueArea) && !overlap(selector.rightArrow, selector.valueArea), name + "arrows apart from value");
    require(selector.leftArrow.x == selector.control.x && selector.rightArrow.x + selector.rightArrow.width == selector.control.x + selector.control.width, name + "arrows at control ends");
    require(layout.rows[2].leftArrow.width == selector.leftArrow.width, name + "toggle same arrows as selector");
}

void checkHits(int w, int h) {
    using namespace qol::ui;
    const std::string name = std::to_string(h) + "p hit ";
    Screen screen = sampleScreen(w, h, 5);
    UiLayout layout = computeLayout(screen, fakeWidth);
    auto centre = [](const qol::Rect& r) { return std::pair<int, int>{r.x + r.width / 2, r.y + r.height / 2}; };

    auto t = centre(layout.tabs[3].rect);
    Hit hit = hitTest(layout, screen, t.first, t.second);
    require(hit.target == Target::Tab && hit.index == 3, name + "tab");
    auto b = centre(layout.backButton);
    require(hitTest(layout, screen, b.first, b.second).target == Target::Back, name + "back");

    auto l = centre(layout.rows[1].leftArrow);
    hit = hitTest(layout, screen, l.first, l.second);
    require(hit.target == Target::Row && hit.index == 1 && hit.part == Part::LeftArrow, name + "left arrow");
    auto r = centre(layout.rows[1].rightArrow);
    require(hitTest(layout, screen, r.first, r.second).part == Part::RightArrow, name + "right arrow");
    auto v = centre(layout.rows[1].valueArea);
    require(hitTest(layout, screen, v.first, v.second).part == Part::Value, name + "selector value");
    auto lab = centre(layout.rows[1].label);
    hit = hitTest(layout, screen, lab.first, lab.second);
    require(hit.target == Target::Row && hit.index == 1 && hit.part == Part::Body, name + "label focuses row");
    auto tr = centre(layout.rows[0].track);
    require(hitTest(layout, screen, tr.first, tr.second).part == Part::Track, name + "slider track");
    auto num = centre(layout.rows[0].numberBox);
    require(hitTest(layout, screen, num.first, num.second).part == Part::Number, name + "slider number");
    auto dd = centre(layout.rows[3].control);
    require(hitTest(layout, screen, dd.first, dd.second).part == Part::Value, name + "dropdown value");
    require(hitTest(layout, screen, 0, h / 2).target == Target::None, name + "outside column");

    const RowLayout& slider = layout.rows[0];
    require(sliderFractionAt(slider, slider.track.x - 50, 10) == 0.0, name + "slider left end");
    require(sliderFractionAt(slider, slider.track.x + slider.track.width + 50, 10) == 1.0, name + "slider right end");
    const int mid = slider.track.x + slider.thumb.width / 2 + (slider.track.width - slider.thumb.width) / 2;
    require(std::fabs(sliderFractionAt(slider, mid, 10) - 0.5) < 1e-9, name + "slider middle snaps to 5 of 10");
    require(std::fabs(sliderFractionAt(slider, mid + 3, 10) - 0.5) < 1e-9, name + "slider snaps to step");
}

void checkScroll(int w, int h) {
    using namespace qol::ui;
    const std::string name = std::to_string(h) + "p scroll ";
    Screen screen = sampleScreen(w, h, 40);
    UiLayout layout = computeLayout(screen, fakeWidth);
    const Scale& s = layout.scale;
    require(layout.scroll.maxOffset > 0, name + "long tab scrolls");
    require(layout.scroll.maxOffset == 40 * s.rowHeight - layout.viewport.height, name + "max offset");
    require(layout.scroll.track.width > 0 && inside(layout.scroll.thumb, layout.scroll.track), name + "scrollbar thumb in track");
    require(!layout.rows[39].visible, name + "last row hidden at top");
    const qol::Rect tabBarBefore = layout.tabBar;
    const qol::Rect hintBefore = layout.hintBar;

    screen.scrollOffset = scrollToShowRow(screen, 39);
    require(screen.scrollOffset == layout.scroll.maxOffset, name + "focus last row scrolls to end");
    layout = computeLayout(screen, fakeWidth);
    require(layout.rows[39].visible && inside(layout.rows[39].rect, layout.viewport), name + "last row fully shown");
    require(!layout.rows[0].visible, name + "first row scrolled out");
    require(layout.tabBar == tabBarBefore && layout.hintBar == hintBefore, name + "bars fixed while scrolling");
    require(layout.scroll.thumb.y + layout.scroll.thumb.height == layout.scroll.track.y + layout.scroll.track.height, name + "thumb at bottom");

    require(scrollToShowRow(screen, 0) == 0, name + "focus first row scrolls back");
    Screen mid = screen;
    mid.scrollOffset = 5 * s.rowHeight;
    require(scrollToShowRow(mid, 6) == mid.scrollOffset, name + "visible row keeps offset");
    require(wheelScroll(screen, 100) == layout.scroll.maxOffset, name + "wheel clamps at end");
    require(wheelScroll(screen, -100) == 0, name + "wheel clamps at start");
    mid.scrollOffset = 0;
    require(wheelScroll(mid, 1) == s.rowHeight, name + "wheel notch is one row");
    require(clampScroll(screen, -5) == 0, name + "clamp low");

    layout = computeLayout(screen, fakeWidth);
    const auto hitBelow = hitTest(layout, screen, w / 2, layout.viewport.y + layout.viewport.height + 1);
    require(hitBelow.target != Target::Row, name + "no row hit under the viewport");
    const auto hitScrollbar = hitTest(layout, screen, layout.scroll.track.x, layout.scroll.track.y + 1);
    require(hitScrollbar.target == Target::Scrollbar, name + "scrollbar hit");

    Screen shortScreen = sampleScreen(w, h, 5);
    const UiLayout shortLayout = computeLayout(shortScreen, fakeWidth);
    require(shortLayout.scroll.maxOffset == 0 && shortLayout.scroll.track.width == 0, name + "short tab has no scrollbar");
    shortScreen.scrollOffset = 999;
    require(computeLayout(shortScreen, fakeWidth).scroll.offset == 0, name + "short tab ignores offset");
}

void checkDropdown(int w, int h) {
    using namespace qol::ui;
    const std::string name = std::to_string(h) + "p dropdown ";
    Screen screen = sampleScreen(w, h, 5);
    screen.openDropdownRow = 3;
    screen.dropdownItemCount = 4;
    UiLayout layout = computeLayout(screen, fakeWidth);
    require(layout.dropdown.open && layout.dropdown.items.size() == 4, name + "four items");
    require(layout.dropdown.popup.y == layout.rows[3].control.y + layout.rows[3].control.height, name + "opens below");
    require(layout.dropdown.popup.x == layout.rows[3].control.x && layout.dropdown.popup.width == layout.rows[3].control.width, name + "as wide as control");
    for (const qol::Rect& item : layout.dropdown.items) require(item.height >= layout.scale.minHit, name + "item hit height");
    auto c = std::pair<int, int>{layout.dropdown.items[2].x + 3, layout.dropdown.items[2].y + 3};
    Hit hit = hitTest(layout, screen, c.first, c.second);
    require(hit.target == Target::DropdownItem && hit.index == 2, name + "item hit");
    auto tab = std::pair<int, int>{layout.tabs[0].rect.x + 2, 2};
    require(hitTest(layout, screen, tab.first, tab.second).target == Target::None, name + "open list blocks other hits");

    screen.dropdownItemCount = 200;
    screen.dropdownFirstItem = 500;
    layout = computeLayout(screen, fakeWidth);
    require(layout.dropdown.items.size() < 200 && !layout.dropdown.items.empty(), name + "long list is limited to room");
    require(layout.dropdown.popup.y + layout.dropdown.popup.height <= h || layout.dropdown.flippedUp, name + "popup stays in window");
    require(layout.dropdown.firstItem == 200 - static_cast<int>(layout.dropdown.items.size()), name + "first item clamped");
    require(layout.dropdown.popup.y >= 0, name + "popup top in window");
}

void checkContrast() {
    using namespace qol::ui;
    const Palette palette;
    for (const ContrastPair& pair : contrastPairs(palette)) {
        const double ratio = contrastRatio(pair.foreground, pair.background);
        require(ratio >= minContrast, std::string("contrast ") + pair.name + " is " + std::to_string(ratio));
    }
    require(std::fabs(contrastRatio({0, 0, 0}, {255, 255, 255}) - 21.0) < 1e-6, "black on white is 21:1");
    require(std::fabs(contrastRatio({255, 255, 255}, {255, 255, 255}) - 1.0) < 1e-9, "same colour is 1:1");
    require(contrastRatio({0x77, 0x77, 0x77}, {255, 255, 255}) >= 4.47 && contrastRatio({0x77, 0x77, 0x77}, {255, 255, 255}) < 4.5, "#777 on white is just below 4.5");
}

}

int main() {
    checkScale(1280, 720, 18.0f, 40, 27);
    checkScale(1920, 1080, 27.0f, 59, 40);
    checkScale(3840, 2160, 54.0f, 119, 80);
    checkScale(2560, 1080, 27.0f, 59, 40);

    const int sizes[][2] = {{1280, 720}, {1920, 1080}, {3840, 2160}, {2560, 1080}, {800, 600}};
    for (const auto& size : sizes) {
        checkLayout(size[0], size[1]);
        checkHits(size[0], size[1]);
        checkScroll(size[0], size[1]);
        checkDropdown(size[0], size[1]);
    }

    const qol::ui::Scale hd = qol::ui::computeScale(1920, 1080);
    require(hd.columnWidth == 1404 && hd.columnX == 258, "1080p column width 1.3 * height, centred");
    require(hd.controlWidth == 632, "1080p control share");
    const qol::ui::Scale narrow = qol::ui::computeScale(800, 600);
    require(narrow.columnWidth == 720, "narrow window limits column to 90 percent of width");
    require(qol::ui::computeLayout(sampleScreen(1920, 1080, 0), fakeWidth).scroll.maxOffset == 0, "empty tab has no scroll");

    checkContrast();

    if (failures != 0) return 1;
    std::cout << "ui layout ok\n";
    return 0;
}
