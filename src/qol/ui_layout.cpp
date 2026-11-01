#include "qol/ui_layout.hpp"

#include <algorithm>
#include <cmath>

namespace qol::ui {

namespace {

int rounded(double value) { return static_cast<int>(std::lround(value)); }

Rect intersect(const Rect& a, const Rect& b) {
    const int x0 = std::max(a.x, b.x);
    const int y0 = std::max(a.y, b.y);
    const int x1 = std::min(a.x + a.width, b.x + b.width);
    const int y1 = std::min(a.y + a.height, b.y + b.height);
    if (x1 <= x0 || y1 <= y0) return {};
    return {x0, y0, x1 - x0, y1 - y0};
}

int headerHeight(const Scale& s, const Screen& screen) { return screen.columnTitles.empty() ? 0 : s.rowHeight; }
int viewportTop(const Scale& s, const Screen& screen) { return s.tabBarHeight + s.margin + headerHeight(s, screen); }
int viewportBottom(const Scale& s) { return s.windowHeight - s.hintBarHeight - s.helpLineHeight - s.margin; }

int contentHeightOf(const Scale& s, std::size_t rows) { return static_cast<int>(rows) * s.rowHeight; }

int maxScrollOf(const Scale& s, const Screen& screen) {
    const int viewport = viewportBottom(s) - viewportTop(s, screen);
    return std::max(0, contentHeightOf(s, screen.rows.size()) - viewport);
}

std::vector<Rect> splitControl(const Rect& c, int count) {
    std::vector<Rect> cells;
    for (int i = 0; i < count; ++i) {
        const int x0 = c.x + c.width * i / count;
        const int x1 = c.x + c.width * (i + 1) / count;
        cells.push_back({x0, c.y, x1 - x0, c.height});
    }
    return cells;
}

double channel(unsigned char value) {
    const double c = value / 255.0;
    return c <= 0.03928 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
}

double luminance(Color c) { return 0.2126 * channel(c.r) + 0.7152 * channel(c.g) + 0.0722 * channel(c.b); }

void layoutControl(RowLayout& row, const RowSpec& spec, const Scale& s, const Proportions& p, const TextMeasure& measure) {
    const Rect& c = row.control;
    switch (spec.kind) {
    case RowKind::Selector:
    case RowKind::Toggle: {
        const int arrow = std::min(s.minHit, c.width / 3);
        row.leftArrow = {c.x, c.y, arrow, c.height};
        row.rightArrow = {c.x + c.width - arrow, c.y, arrow, c.height};
        row.valueArea = {c.x + arrow, c.y, c.width - 2 * arrow, c.height};
        row.valueFits = measure(spec.value, s.textPixels) <= static_cast<float>(row.valueArea.width - s.padding);
        break;
    }
    case RowKind::Slider: {
        const int number = rounded(measure("100", s.textPixels)) + 2 * s.padding;
        row.numberBox = {c.x + c.width - number, c.y, number, c.height};
        const int trackWidth = c.width - number - s.margin;
        const int trackHeight = std::max(2, rounded(c.height * p.sliderTrackOfRow));
        row.track = {c.x, c.y + (c.height - trackHeight) / 2, trackWidth, trackHeight};
        const int thumbSize = std::max(trackHeight, rounded(c.height * p.sliderThumbOfRow));
        const double f = std::clamp(spec.fraction, 0.0, 1.0);
        row.thumb = {c.x + rounded(f * (trackWidth - thumbSize)), c.y + (c.height - thumbSize) / 2, thumbSize, thumbSize};
        row.valueFits = measure(spec.value, s.textPixels) <= static_cast<float>(number - s.padding);
        break;
    }
    case RowKind::Dropdown:
        row.valueArea = c;
        row.valueFits = measure(spec.value, s.textPixels) <= static_cast<float>(c.width - 2 * s.minHit);
        break;
    case RowKind::Button:
        row.valueArea = c;
        row.valueFits = measure(spec.value, s.textPixels) <= static_cast<float>(c.width - s.padding);
        row.cells = splitControl(c, static_cast<int>(spec.cells.size()));
        for (std::size_t i = 0; i < row.cells.size(); ++i)
            row.cellFits.push_back(measure(spec.cells[i], s.textPixels) <= static_cast<float>(row.cells[i].width - s.padding));
        break;
    }
}

}

Scale computeScale(int windowWidth, int windowHeight, const Proportions& p) {
    Scale s;
    s.windowWidth = windowWidth;
    s.windowHeight = windowHeight;
    s.textPixels = static_cast<float>(windowHeight * textHeightFraction);
    s.minHit = rounded(static_cast<double>(minHitPixelsAt1080) * windowHeight / referenceHeight);
    s.rowHeight = std::max(rounded(windowHeight * rowHeightFraction), s.minHit);
    s.margin = rounded(s.rowHeight * p.marginOfRow);
    s.padding = rounded(s.textPixels * p.paddingOfText);
    s.underline = std::max(2, rounded(windowHeight * p.underlineOfHeight));
    s.border = std::max(2, rounded(windowHeight * p.borderOfHeight));
    s.tabBarHeight = s.rowHeight;
    s.helpLineHeight = s.rowHeight;
    s.hintBarHeight = s.rowHeight;
    s.columnWidth = std::min(rounded(windowHeight * p.columnWidthOfHeight), rounded(windowWidth * p.columnWidthOfWidthMax));
    s.columnX = (windowWidth - s.columnWidth) / 2;
    s.controlWidth = rounded(s.columnWidth * p.controlShareOfColumn);
    return s;
}

int clampScroll(const Screen& screen, int offset, const Proportions& p) {
    const Scale s = computeScale(screen.windowWidth, screen.windowHeight, p);
    return std::clamp(offset, 0, maxScrollOf(s, screen));
}

int scrollToShowRow(const Screen& screen, int row, const Proportions& p) {
    const Scale s = computeScale(screen.windowWidth, screen.windowHeight, p);
    int offset = screen.scrollOffset;
    if (row >= 0 && row < static_cast<int>(screen.rows.size())) {
        const int viewport = viewportBottom(s) - viewportTop(s, screen);
        const int top = row * s.rowHeight;
        if (top < offset) offset = top;
        if (top + s.rowHeight > offset + viewport) offset = top + s.rowHeight - viewport;
    }
    return std::clamp(offset, 0, maxScrollOf(s, screen));
}

int wheelScroll(const Screen& screen, int notches, const Proportions& p) {
    const Scale s = computeScale(screen.windowWidth, screen.windowHeight, p);
    return std::clamp(screen.scrollOffset + notches * s.rowHeight, 0, maxScrollOf(s, screen));
}

UiLayout computeLayout(const Screen& screen, const TextMeasure& measure, const Proportions& p) {
    UiLayout out;
    const Scale s = computeScale(screen.windowWidth, screen.windowHeight, p);
    out.scale = s;

    out.tabBar = {0, 0, s.windowWidth, s.tabBarHeight};
    std::vector<int> widths;
    int total = 0;
    for (const std::string& tab : screen.tabs) {
        widths.push_back(std::max(rounded(measure(tab, s.textPixels)) + 2 * s.padding, s.minHit));
        total += widths.back();
    }
    int x = (s.windowWidth - total) / 2;
    for (std::size_t i = 0; i < screen.tabs.size(); ++i) {
        TabLayout tab;
        tab.rect = {x, 0, widths[i], s.tabBarHeight};
        tab.selected = static_cast<int>(i) == screen.selectedTab;
        tab.focused = tab.selected && screen.focusOnTabs;
        if (tab.selected) tab.underline = {x, s.tabBarHeight - s.underline, widths[i], s.underline};
        tab.labelFits = rounded(measure(screen.tabs[i], s.textPixels)) + s.padding <= widths[i];
        out.tabs.push_back(tab);
        x += widths[i];
    }

    const int top = viewportTop(s, screen);
    const int bottom = viewportBottom(s);
    const int labelWidth = s.columnWidth - s.controlWidth - s.margin;
    if (!screen.columnTitles.empty()) {
        out.header = {s.columnX, top - headerHeight(s, screen), s.columnWidth, s.rowHeight};
        out.headerCells.push_back({out.header.x + s.padding, out.header.y, labelWidth - s.padding, s.rowHeight});
        const Rect control{out.header.x + s.columnWidth - s.controlWidth - s.padding, out.header.y, s.controlWidth, s.rowHeight};
        for (const Rect& cell : splitControl(control, static_cast<int>(screen.columnTitles.size()) - 1)) out.headerCells.push_back(cell);
    }
    out.viewport = {s.columnX, top, s.columnWidth, std::max(0, bottom - top)};
    out.helpLine = {s.columnX, s.windowHeight - s.hintBarHeight - s.helpLineHeight, s.columnWidth, s.helpLineHeight};
    out.hintBar = {0, s.windowHeight - s.hintBarHeight, s.windowWidth, s.hintBarHeight};
    const int backWidth = std::max(rounded(measure(screen.backText, s.textPixels)) + 2 * s.padding, 2 * s.minHit);
    out.backButton = {s.margin, out.hintBar.y, backWidth, s.hintBarHeight};
    out.hintText = {out.backButton.x + backWidth + s.margin, out.hintBar.y,
                    std::max(0, s.windowWidth - backWidth - 3 * s.margin), s.hintBarHeight};

    const int maxOffset = maxScrollOf(s, screen);
    const int offset = std::clamp(screen.scrollOffset, 0, maxOffset);
    out.scroll.offset = offset;
    out.scroll.maxOffset = maxOffset;
    out.scroll.contentHeight = contentHeightOf(s, screen.rows.size());
    if (maxOffset > 0) {
        const int width = std::max(2, rounded(s.rowHeight * p.scrollbarOfRow));
        out.scroll.track = {s.columnX + s.columnWidth + s.margin, top, width, out.viewport.height};
        const int thumbHeight = std::max(s.minHit, out.viewport.height * out.viewport.height / out.scroll.contentHeight);
        const int travel = out.viewport.height - thumbHeight;
        out.scroll.thumb = {out.scroll.track.x, top + travel * offset / maxOffset, width, thumbHeight};
    }

    for (std::size_t i = 0; i < screen.rows.size(); ++i) {
        const RowSpec& spec = screen.rows[i];
        RowLayout row;
        row.rect = {s.columnX, top + static_cast<int>(i) * s.rowHeight - offset, s.columnWidth, s.rowHeight};
        row.visible = intersect(row.rect, out.viewport).height > 0;
        row.focused = static_cast<int>(i) == screen.focusRow && !screen.focusOnTabs;
        row.label = {row.rect.x + s.padding, row.rect.y, labelWidth - s.padding, s.rowHeight};
        row.control = {row.rect.x + s.columnWidth - s.controlWidth - s.padding, row.rect.y, s.controlWidth, s.rowHeight};
        row.labelFits = measure(spec.label, s.textPixels) <= static_cast<float>(row.label.width);
        layoutControl(row, spec, s, p, measure);
        if (row.focused && !row.cells.empty())
            row.focusedCell = std::clamp(screen.focusCell, 0, static_cast<int>(row.cells.size()) - 1);
        if (row.focused) {
            row.focusBar = row.rect;
            row.focusBorder = row.rect;
        }
        out.rows.push_back(row);
    }

    if (screen.openDropdownRow >= 0 && screen.openDropdownRow < static_cast<int>(out.rows.size()) && screen.dropdownItemCount > 0) {
        const Rect anchor = out.rows[static_cast<std::size_t>(screen.openDropdownRow)].control;
        const int below = s.windowHeight - s.hintBarHeight - (anchor.y + anchor.height);
        const int above = anchor.y - s.tabBarHeight;
        const int wanted = screen.dropdownItemCount * s.rowHeight;
        const bool flip = below < wanted && above > below;
        const int room = flip ? above : below;
        const int visible = std::max(1, std::min(screen.dropdownItemCount, room / s.rowHeight));
        out.dropdown.open = true;
        out.dropdown.flippedUp = flip;
        out.dropdown.firstItem = std::clamp(screen.dropdownFirstItem, 0, screen.dropdownItemCount - visible);
        const int height = visible * s.rowHeight;
        out.dropdown.popup = {anchor.x, flip ? anchor.y - height : anchor.y + anchor.height, anchor.width, height};
        for (int i = 0; i < visible; ++i)
            out.dropdown.items.push_back({anchor.x, out.dropdown.popup.y + i * s.rowHeight, anchor.width, s.rowHeight});
    }
    return out;
}

bool contains(const Rect& r, int x, int y) {
    return x >= r.x && y >= r.y && x < r.x + r.width && y < r.y + r.height;
}

Hit hitTest(const UiLayout& layout, const Screen& screen, int x, int y) {
    if (layout.dropdown.open) {
        for (std::size_t i = 0; i < layout.dropdown.items.size(); ++i)
            if (contains(layout.dropdown.items[i], x, y))
                return {Target::DropdownItem, Part::Body, layout.dropdown.firstItem + static_cast<int>(i)};
        return {};
    }
    if (contains(layout.backButton, x, y)) return {Target::Back, Part::Body, 0};
    for (std::size_t i = 0; i < layout.tabs.size(); ++i)
        if (contains(layout.tabs[i].rect, x, y)) return {Target::Tab, Part::Body, static_cast<int>(i)};
    if (layout.scroll.maxOffset > 0 && contains(layout.scroll.track, x, y)) return {Target::Scrollbar, Part::Body, 0};
    if (!contains(layout.viewport, x, y)) return {};
    for (std::size_t i = 0; i < layout.rows.size(); ++i) {
        const RowLayout& row = layout.rows[i];
        if (!contains(row.rect, x, y)) continue;
        Part part = Part::Body;
        switch (screen.rows[i].kind) {
        case RowKind::Selector:
        case RowKind::Toggle:
            if (contains(row.leftArrow, x, y)) part = Part::LeftArrow;
            else if (contains(row.rightArrow, x, y)) part = Part::RightArrow;
            else if (contains(row.valueArea, x, y)) part = Part::Value;
            break;
        case RowKind::Slider:
            if (x >= row.control.x && x < row.numberBox.x) part = Part::Track;
            else if (contains(row.numberBox, x, y)) part = Part::Number;
            break;
        case RowKind::Dropdown:
        case RowKind::Button:
            if (contains(row.control, x, y)) part = Part::Value;
            for (std::size_t cell = 0; cell < row.cells.size(); ++cell)
                if (contains(row.cells[cell], x, y)) return {Target::Row, Part::Cell, static_cast<int>(i), static_cast<int>(cell)};
            break;
        }
        return {Target::Row, part, static_cast<int>(i)};
    }
    return {};
}

double sliderFractionAt(const RowLayout& row, int x, int steps) {
    const int span = row.track.width - row.thumb.width;
    if (span <= 0) return 0;
    const double f = std::clamp(static_cast<double>(x - row.thumb.width / 2 - row.track.x) / span, 0.0, 1.0);
    if (steps <= 0) return f;
    return std::round(f * steps) / steps;
}

double contrastRatio(Color a, Color b) {
    const double la = luminance(a);
    const double lb = luminance(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

std::vector<ContrastPair> contrastPairs(const Palette& p) {
    return {
        {"text on panel", p.text, p.panel},
        {"disabled text on panel", p.textDisabled, p.panel},
        {"text on control", p.text, p.control},
        {"focus text on focus bar", p.focusText, p.focusBar},
        {"focus border on panel", p.focusBorder, p.panel},
        {"slider track on panel", p.sliderTrack, p.panel},
        {"slider fill on panel", p.sliderFill, p.panel},
        {"tab underline on panel", p.tabUnderline, p.panel},
    };
}

}
