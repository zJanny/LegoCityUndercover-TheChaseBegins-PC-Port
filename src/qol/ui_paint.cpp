#include "qol/ui_paint.hpp"

#include "render/ui_font.hpp"

#include <algorithm>
#include <cmath>

namespace qol::ui {

namespace {

using chase::render::UiClip;
using chase::render::UiColour;
using chase::render::UiDrawList;
using chase::render::UiRect;

enum class Align { Left, Center };
enum class Arrow { Left, Right, Down };

constexpr float dialogWidthOfColumn = 0.8f;
constexpr float bodyLineSpacing = 1.25f;
constexpr double controlInsetOfRow = 0.1;
constexpr double cornerOfRow = 0.12;
constexpr double chevronOfHeight = 0.14;

int rounded(double value) { return static_cast<int>(std::lround(value)); }

UiColour colour(Color c, float alpha = 1.0f) {
    return {static_cast<float>(c.r) / 255.0f, static_cast<float>(c.g) / 255.0f, static_cast<float>(c.b) / 255.0f, alpha};
}

UiRect uiRect(const Rect& r) {
    return {static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.width), static_cast<float>(r.height)};
}

UiClip uiClip(const Rect& r) { return {r.x, r.y, r.width, r.height}; }

TextMeasure measureOf(const MenuChrome& chrome) {
    if (chrome.measure) return chrome.measure;
    return [](std::string_view text, float pixelSize) { return chase::render::uiTextWidth(text, pixelSize); };
}

LineHeight lineHeightOf(const MenuChrome& chrome) {
    if (chrome.lineHeight) return chrome.lineHeight;
    return [](float pixelSize) { return chase::render::uiFontMetrics(pixelSize).lineHeight; };
}

std::vector<std::string> wrap(const std::string& text, float pixelSize, float width, const TextMeasure& measure) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t end = std::min(text.find('\n', start), text.size());
        const std::string paragraph = text.substr(start, end - start);
        std::string line;
        std::size_t at = 0;
        while (at < paragraph.size()) {
            const std::size_t space = std::min(paragraph.find(' ', at), paragraph.size());
            const std::string word = paragraph.substr(at, space - at);
            const std::string candidate = line.empty() ? word : line + " " + word;
            if (!line.empty() && measure(candidate, pixelSize) > width) {
                lines.push_back(line);
                line = word;
            } else {
                line = candidate;
            }
            at = space + 1;
        }
        if (!line.empty()) lines.push_back(line);
        start = end + 1;
    }
    return lines;
}

class Painter {
public:
    Painter(UiDrawList& list, const Screen& screen, const UiLayout& layout, const MenuChrome& chrome)
        : list_(list), screen_(screen), layout_(layout), chrome_(chrome), p_(chrome.palette), s_(layout.scale),
          measure_(measureOf(chrome)), lineHeight_(lineHeightOf(chrome)),
          corner_(static_cast<float>(layout.scale.rowHeight * cornerOfRow)),
          inset_(std::max(2, rounded(layout.scale.rowHeight * controlInsetOfRow))) {}

    void paint() {
        list_.dim({0, 0, 0, chrome_.dimAlpha});
        panels();
        tabs();
        rows();
        scrollbar();
        helpLine();
        hintBar();
        dropdownPopup();
        if (chrome_.dialog) dialog(*chrome_.dialog);
    }

private:
    Rect padded(const Rect& r) const { return {r.x + s_.padding, r.y, std::max(0, r.width - s_.padding), r.height}; }

    Rect inner(const Rect& r) const { return {r.x, r.y + inset_, r.width, std::max(0, r.height - 2 * inset_)}; }

    void fill(const Rect& r, Color c, float radius = 0) { list_.fill(uiRect(r), colour(c), radius); }

    void text(const std::string& str, const Rect& cell, Align align, Color c) {
        if (str.empty() || cell.width <= 0 || cell.height <= 0) return;
        const float pixels = s_.textPixels;
        const float width = measure_(str, pixels);
        const float x = align == Align::Left ? static_cast<float>(cell.x) : static_cast<float>(cell.x) + std::max(0.0f, (static_cast<float>(cell.width) - width) / 2.0f);
        const float y = static_cast<float>(cell.y) + (static_cast<float>(cell.height) - lineHeight_(pixels)) / 2.0f;
        if (bound_ && !lineMeetsBound(cell, y, lineHeight_(pixels))) return;
        list_.pushClip(uiClip(cell));
        list_.text(str, pixels, x, y, colour(c));
        list_.popClip();
    }

    bool lineMeetsBound(const Rect& cell, float y, float height) const {
        const int top = std::max(cell.y, bound_->y);
        const int bottom = std::min(cell.y + cell.height, bound_->y + bound_->height);
        return bottom > top && y < static_cast<float>(bottom) && y + height > static_cast<float>(top);
    }

    void chevron(const Rect& cell, Arrow arrow, Color c) {
        const float cx = static_cast<float>(cell.x) + static_cast<float>(cell.width) / 2.0f;
        const float cy = static_cast<float>(cell.y) + static_cast<float>(cell.height) / 2.0f;
        const float h = std::max(2.0f, static_cast<float>(cell.height * chevronOfHeight));
        const float t = static_cast<float>(s_.border);
        const UiColour col = colour(c);
        switch (arrow) {
        case Arrow::Left:
            list_.line(cx + h / 2, cy - h, cx - h / 2, cy, t, col);
            list_.line(cx - h / 2, cy, cx + h / 2, cy + h, t, col);
            break;
        case Arrow::Right:
            list_.line(cx - h / 2, cy - h, cx + h / 2, cy, t, col);
            list_.line(cx + h / 2, cy, cx - h / 2, cy + h, t, col);
            break;
        case Arrow::Down:
            list_.line(cx - h, cy - h / 2, cx, cy + h / 2, t, col);
            list_.line(cx, cy + h / 2, cx + h, cy - h / 2, t, col);
            break;
        }
    }

    void panels() {
        fill(layout_.tabBar, p_.panel);
        fill(layout_.hintBar, p_.panel);
        const int top = layout_.tabBar.height;
        const int bottom = layout_.hintBar.y;
        int left = s_.columnX - s_.margin;
        int right = s_.columnX + s_.columnWidth + s_.margin;
        if (layout_.scroll.maxOffset > 0) right = std::max(right, layout_.scroll.track.x + layout_.scroll.track.width + s_.margin);
        left = std::max(0, left);
        right = std::min(s_.windowWidth, right);
        fill({left, top, std::max(0, right - left), std::max(0, bottom - top)}, p_.panel);
    }

    void tabs() {
        for (std::size_t i = 0; i < layout_.tabs.size() && i < screen_.tabs.size(); ++i) {
            const TabLayout& tab = layout_.tabs[i];
            if (tab.focused) {
                fill(tab.rect, p_.focusBar);
                list_.border(uiRect(tab.rect), colour(p_.focusBorder), static_cast<float>(s_.border));
            }
            text(screen_.tabs[i], tab.rect, Align::Center, tab.focused ? p_.focusText : p_.text);
            if (tab.selected) fill(tab.underline, p_.tabUnderline);
        }
    }

    void header() {
        for (std::size_t i = 0; i < layout_.headerCells.size() && i < screen_.columnTitles.size(); ++i)
            text(screen_.columnTitles[i], layout_.headerCells[i], i == 0 ? Align::Left : Align::Center, p_.textDisabled);
    }

    void rows() {
        header();
        list_.pushClip(uiClip(layout_.viewport));
        bound_ = layout_.viewport;
        for (std::size_t i = 0; i < layout_.rows.size() && i < screen_.rows.size(); ++i)
            row(screen_.rows[i], layout_.rows[i], static_cast<int>(i) == screen_.openDropdownRow);
        bound_.reset();
        list_.popClip();
    }

    void row(const RowSpec& spec, const RowLayout& layout, bool dropdownOpen) {
        if (!layout.visible) return;
        if (layout.focused) {
            fill(layout.focusBar, p_.focusBar);
            list_.border(uiRect(layout.focusBorder), colour(p_.focusBorder), static_cast<float>(s_.border));
        }
        text(spec.label, layout.label, Align::Left, layout.focused ? p_.focusText : spec.enabled ? p_.text : p_.textDisabled);
        control(spec, layout, spec.enabled ? p_.text : p_.textDisabled, dropdownOpen);
    }

    void control(const RowSpec& spec, const RowLayout& layout, Color valueColour, bool dropdownOpen) {
        switch (spec.kind) {
        case RowKind::Selector:
        case RowKind::Toggle:
            fill(inner(layout.control), p_.control, corner_);
            chevron(layout.leftArrow, Arrow::Left, valueColour);
            chevron(layout.rightArrow, Arrow::Right, valueColour);
            text(spec.value, layout.valueArea, Align::Center, valueColour);
            break;
        case RowKind::Slider:
            fill(layout.track, p_.sliderTrack, static_cast<float>(layout.track.height) / 2.0f);
            if (spec.enabled) {
                const int filled = layout.thumb.x + layout.thumb.width / 2 - layout.track.x;
                if (filled > 0) fill({layout.track.x, layout.track.y, filled, layout.track.height}, p_.sliderFill, static_cast<float>(layout.track.height) / 2.0f);
            }
            fill(layout.thumb, spec.enabled ? p_.text : p_.textDisabled, static_cast<float>(layout.thumb.width) / 2.0f);
            fill(inner(layout.numberBox), p_.control, corner_);
            text(spec.value, layout.numberBox, Align::Center, valueColour);
            break;
        case RowKind::Dropdown: {
            const Rect box = inner(layout.control);
            fill(box, p_.control, corner_);
            if (dropdownOpen) list_.border(uiRect(box), colour(p_.focusBorder), static_cast<float>(s_.border), corner_);
            const Rect arrow{layout.control.x + layout.control.width - s_.minHit, layout.control.y, s_.minHit, layout.control.height};
            text(spec.value, padded({layout.control.x, layout.control.y, layout.control.width - s_.minHit, layout.control.height}), Align::Left, valueColour);
            chevron(arrow, Arrow::Down, valueColour);
            break;
        }
        case RowKind::Button:
            if (!layout.cells.empty()) {
                cells(spec, layout, valueColour);
                break;
            }
            fill(inner(layout.control), p_.control, corner_);
            text(spec.value, layout.control, Align::Center, valueColour);
            break;
        }
    }

    void cells(const RowSpec& spec, const RowLayout& layout, Color valueColour) {
        for (std::size_t i = 0; i < layout.cells.size() && i < spec.cells.size(); ++i) {
            const Rect& cell = layout.cells[i];
            const Rect box = inner({cell.x + inset_, cell.y, std::max(0, cell.width - 2 * inset_), cell.height});
            fill(box, p_.control, corner_);
            if (static_cast<int>(i) == layout.focusedCell)
                list_.border(uiRect(box), colour(p_.focusBorder), static_cast<float>(s_.border), corner_);
            text(spec.cells[i], box, Align::Center, valueColour);
        }
    }

    void scrollbar() {
        if (layout_.scroll.maxOffset <= 0) return;
        const float radius = static_cast<float>(layout_.scroll.track.width) / 2.0f;
        fill(layout_.scroll.track, p_.control, radius);
        fill(layout_.scroll.thumb, p_.sliderTrack, radius);
    }

    void helpLine() { text(chrome_.helpText, padded(layout_.helpLine), Align::Left, p_.text); }

    void hintBar() {
        fill(inner(layout_.backButton), p_.control, corner_);
        text(screen_.backText, layout_.backButton, Align::Center, p_.text);
        text(chrome_.hintText.empty() ? defaultHintText(chrome_.device) : chrome_.hintText, layout_.hintText, Align::Left, p_.text);
    }

    void dropdownPopup() {
        const DropdownLayout& d = layout_.dropdown;
        if (!d.open) return;
        fill(d.popup, p_.control, corner_);
        for (std::size_t i = 0; i < d.items.size(); ++i) {
            const int index = d.firstItem + static_cast<int>(i);
            const bool focused = index == chrome_.dropdownFocus;
            if (focused) fill(d.items[i], p_.focusBar);
            if (index == chrome_.dropdownSelected) fill({d.items[i].x, d.items[i].y, 2 * s_.border, d.items[i].height}, p_.sliderFill);
            const std::string& label = index >= 0 && index < static_cast<int>(chrome_.dropdownItems.size()) ? chrome_.dropdownItems[static_cast<std::size_t>(index)] : std::string();
            text(label, padded({d.items[i].x + 2 * s_.border, d.items[i].y, d.items[i].width - 2 * s_.border, d.items[i].height}), Align::Left, focused ? p_.focusText : p_.text);
        }
        list_.border(uiRect(d.popup), colour(p_.focusBorder), static_cast<float>(s_.border), corner_);
    }

    void dialog(const MenuDialog& dialog) {
        list_.dim({0, 0, 0, chrome_.dialogDimAlpha});
        const DialogLayout d = computeDialogLayout(dialog, s_, measure_, lineHeight_);
        fill(d.box, p_.panel, corner_);
        list_.border(uiRect(d.box), colour(p_.focusBorder), static_cast<float>(s_.border), corner_);
        text(dialog.title, d.title, Align::Center, p_.text);
        for (std::size_t i = 0; i < d.bodyLines.size(); ++i) text(d.bodyLines[i], d.bodyRects[i], Align::Center, p_.text);
        text(dialog.countdownText, d.countdown, Align::Center, p_.text);
        for (std::size_t i = 0; i < d.buttons.size(); ++i) {
            const bool focused = static_cast<int>(i) == dialog.focusedButton;
            fill(d.buttons[i], focused ? p_.focusBar : p_.control, corner_);
            if (focused) list_.border(uiRect(d.buttons[i]), colour(p_.focusBorder), static_cast<float>(s_.border), corner_);
            text(dialog.buttons[i], d.buttons[i], Align::Center, focused ? p_.focusText : p_.text);
        }
    }

    UiDrawList& list_;
    const Screen& screen_;
    const UiLayout& layout_;
    const MenuChrome& chrome_;
    const Palette& p_;
    const Scale& s_;
    TextMeasure measure_;
    LineHeight lineHeight_;
    float corner_;
    int inset_;
    std::optional<Rect> bound_;
};

}

std::string defaultHintText(InputDevice device) {
    switch (device) {
    case InputDevice::Keyboard: return "Up/Down move  Left/Right change  Enter select  Esc back";
    case InputDevice::Mouse: return "Click select  Wheel scroll  Right-click back";
    case InputDevice::Pad: return "D-pad move  A select  B back  LB/RB tabs";
    }
    return {};
}

DialogLayout computeDialogLayout(const MenuDialog& dialog, const Scale& s, const TextMeasure& measure, const LineHeight& lineHeight) {
    DialogLayout out;
    const int pad = s.margin;
    const int boxWidth = std::min(rounded(s.columnWidth * dialogWidthOfColumn), s.windowWidth - 2 * s.margin);
    const int innerWidth = std::max(0, boxWidth - 2 * pad);
    const int bodyLine = rounded(lineHeight(s.textPixels) * bodyLineSpacing);
    out.bodyLines = wrap(dialog.body, s.textPixels, static_cast<float>(innerWidth), measure);

    const int bodyHeight = static_cast<int>(out.bodyLines.size()) * bodyLine;
    const int countdownHeight = dialog.countdownText.empty() ? 0 : s.rowHeight;
    const int buttonsHeight = dialog.buttons.empty() ? 0 : pad + s.rowHeight;
    const int height = pad + s.rowHeight + bodyHeight + countdownHeight + buttonsHeight + pad;
    out.box = {(s.windowWidth - boxWidth) / 2, std::max(0, (s.windowHeight - height) / 2), boxWidth, height};

    const int x = out.box.x + pad;
    int y = out.box.y + pad;
    out.title = {x, y, innerWidth, s.rowHeight};
    y += s.rowHeight;
    for (std::size_t i = 0; i < out.bodyLines.size(); ++i) {
        out.bodyRects.push_back({x, y, innerWidth, bodyLine});
        y += bodyLine;
    }
    out.countdown = {x, y, innerWidth, countdownHeight};
    y += countdownHeight;

    const int count = static_cast<int>(dialog.buttons.size());
    if (count > 0) {
        std::vector<int> widths;
        int total = (count - 1) * s.margin;
        for (const std::string& label : dialog.buttons) {
            widths.push_back(std::max(rounded(measure(label, s.textPixels)) + 2 * s.padding, 2 * s.minHit));
            total += widths.back();
        }
        if (total > innerWidth) {
            const int each = std::max(1, (innerWidth - (count - 1) * s.margin) / count);
            std::fill(widths.begin(), widths.end(), each);
            total = count * each + (count - 1) * s.margin;
        }
        int bx = out.box.x + (boxWidth - total) / 2;
        const int by = out.box.y + height - pad - s.rowHeight;
        for (int w : widths) {
            out.buttons.push_back({bx, by, w, s.rowHeight});
            bx += w + s.margin;
        }
    }
    return out;
}

int hitDialogButton(const DialogLayout& layout, int x, int y) {
    for (std::size_t i = 0; i < layout.buttons.size(); ++i)
        if (contains(layout.buttons[i], x, y)) return static_cast<int>(i);
    return -1;
}

void paintMenu(UiDrawList& list, const Screen& screen, const UiLayout& layout, const MenuChrome& chrome) {
    Painter(list, screen, layout, chrome).paint();
}

}
