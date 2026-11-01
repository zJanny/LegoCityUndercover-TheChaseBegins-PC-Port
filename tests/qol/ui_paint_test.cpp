// Spec: docs/design/qol-settings-drawing.md entry 2026-10-30T21:56:04 (full-window overlay over the dimmed game picture, top tabs
// with the selected tab underlined, rows with the label left and the control right, focus by bar plus border, help line of the
// focused row, key-hint bar that follows the input device, Back button bottom-left, scrolling rows with fixed tab and hint bars,
// dropdown list, modal dialogs; contrast at least 4.5:1) and answer 10 (proportions). Expectations written from the spec and
// from the rectangles of the layout model by 20261101-qolui3-C on 2026-11-01. Text width is a fixed-width fake (0.5 * pixel size
// per character) except in the font checks, which use the bundled font.
#include "qol/ui_page_qol.hpp"
#include "qol/ui_paint.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace {

using namespace qol::ui;
using chase::render::UiBatch;
using chase::render::UiColour;
using chase::render::UiCommand;
using chase::render::UiCommandKind;
using chase::render::UiDrawList;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

float fakeWidth(std::string_view text, float pixelSize) { return 0.5f * pixelSize * static_cast<float>(text.size()); }
float fakeLine(float pixelSize) { return 1.2f * pixelSize; }

struct Size {
    int width;
    int height;
};

const Size sizes[] = {{1280, 720}, {1920, 1080}, {3840, 2160}};

UiColour colourOf(Color c) { return {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, 1.0f}; }

bool sameColour(const UiColour& a, const UiColour& b) {
    return std::fabs(a.r - b.r) < 1e-5f && std::fabs(a.g - b.g) < 1e-5f && std::fabs(a.b - b.b) < 1e-5f;
}

bool sameRect(const UiCommand& c, const qol::Rect& r) {
    return c.rect.x == static_cast<float>(r.x) && c.rect.y == static_cast<float>(r.y) &&
           c.rect.width == static_cast<float>(r.width) && c.rect.height == static_cast<float>(r.height);
}

int countOf(const UiDrawList& list, UiCommandKind kind) {
    int n = 0;
    for (const UiCommand& c : list.commands()) n += c.kind == kind;
    return n;
}

int countRect(const UiDrawList& list, UiCommandKind kind, const qol::Rect& rect, const Color* colour = nullptr) {
    int n = 0;
    for (const UiCommand& c : list.commands())
        if (c.kind == kind && sameRect(c, rect) && (!colour || sameColour(c.colour, colourOf(*colour)))) ++n;
    return n;
}

int countText(const UiDrawList& list, const std::string& text) {
    int n = 0;
    for (const UiCommand& c : list.commands()) n += c.kind == UiCommandKind::Text && c.text == text;
    return n;
}

bool insideRect(const qol::Rect& inner, const qol::Rect& outer) {
    return inner.x >= outer.x && inner.y >= outer.y && inner.x + inner.width <= outer.x + outer.width &&
           inner.y + inner.height <= outer.y + outer.height;
}

bool overlaps(const qol::Rect& a, const qol::Rect& b) {
    return a.x < b.x + b.width && b.x < a.x + a.width && a.y < b.y + b.height && b.y < a.y + a.height;
}

qol::Rect clipRect(const UiCommand& c) { return {c.clip.x, c.clip.y, c.clip.width, c.clip.height}; }

Screen sampleScreen(int w, int h, std::size_t rowCount = 6) {
    Screen s;
    s.windowWidth = w;
    s.windowHeight = h;
    s.tabs = {"Game", "Display", "Controls", "Quality of life"};
    const RowKind kinds[] = {RowKind::Slider, RowKind::Selector, RowKind::Toggle, RowKind::Dropdown, RowKind::Button, RowKind::Selector};
    const char* values[] = {"70", "Fullscreen", "On", "1920x1080", "Reset", "Locked"};
    for (std::size_t i = 0; i < rowCount; ++i)
        s.rows.push_back({"Setting " + std::to_string(i), kinds[i % 6], values[i % 6], 0.7, i % 6 != 5});
    s.focusRow = 2;
    return s;
}

MenuChrome sampleChrome() {
    MenuChrome chrome;
    chrome.helpText = "Help of the focused row. Default: Off";
    chrome.measure = fakeWidth;
    chrome.lineHeight = fakeLine;
    return chrome;
}

UiDrawList paint(const Screen& screen, const MenuChrome& chrome, UiLayout* layoutOut = nullptr) {
    const UiLayout layout = computeLayout(screen, fakeWidth);
    UiDrawList list;
    paintMenu(list, screen, layout, chrome);
    if (layoutOut) *layoutOut = layout;
    return list;
}

void checkBalancedAndClipped(const UiDrawList& list0, const Screen& screen, const UiLayout& layout, const std::string& name) {
    UiDrawList list = list0;
    list.fill({0, 0, 1, 1}, {1, 1, 1, 1});
    require(!list.commands().back().clipped, name + "clip stack is balanced");
    const qol::Rect window{0, 0, screen.windowWidth, screen.windowHeight};
    std::vector<qol::Rect> regions{layout.tabBar, layout.viewport, layout.helpLine, layout.hintBar};
    if (layout.dropdown.open) regions.push_back(layout.dropdown.popup);
    for (const UiCommand& c : list0.commands()) {
        if (c.kind != UiCommandKind::Text) continue;
        require(c.clipped, name + "text is clipped");
        const qol::Rect clip = clipRect(c);
        require(clip.width > 0 && clip.height > 0 && insideRect(clip, window), name + "text clip inside the window: " + c.text);
        const bool inRegion = std::any_of(regions.begin(), regions.end(), [&](const qol::Rect& r) { return insideRect(clip, r); });
        require(inRegion, name + "text clip inside one region: " + c.text);
        require(c.rect.x >= static_cast<float>(clip.x) - 0.5f && c.rect.x <= static_cast<float>(clip.x + clip.width), name + "text starts in its clip: " + c.text);
        require(c.rect.y <= static_cast<float>(clip.y + clip.height) && c.rect.y + fakeLine(c.pixelSize) >= static_cast<float>(clip.y), name + "text line meets its clip: " + c.text);
    }
}

void checkChrome(const Size& size) {
    const std::string name = std::to_string(size.height) + "p ";
    Screen screen = sampleScreen(size.width, size.height);
    MenuChrome chrome = sampleChrome();
    UiLayout layout;
    const UiDrawList list = paint(screen, chrome, &layout);
    const Palette& p = chrome.palette;

    const UiCommand& first = list.commands().front();
    require(first.kind == UiCommandKind::Dim && !first.clipped && std::fabs(first.colour.a - chrome.dimAlpha) < 1e-6f, name + "dimmed game picture first");
    require(countOf(list, UiCommandKind::Dim) == 1, name + "one dim without a dialog");

    require(countRect(list, UiCommandKind::Fill, layout.tabBar, &p.panel) == 1, name + "tab bar panel");
    require(countRect(list, UiCommandKind::Fill, layout.hintBar, &p.panel) == 1, name + "hint bar panel");
    for (std::size_t i = 0; i < layout.tabs.size(); ++i) {
        require(countText(list, screen.tabs[i]) == 1, name + "tab label " + screen.tabs[i]);
        const int underline = countRect(list, UiCommandKind::Fill, layout.tabs[i].underline, &p.tabUnderline);
        require(underline == (layout.tabs[i].selected ? 1 : 0), name + "only the selected tab is underlined");
        require(countRect(list, UiCommandKind::Border, layout.tabs[i].rect) == 0, name + "no tab border while the rows have focus");
    }

    const RowLayout& focused = layout.rows[2];
    require(countRect(list, UiCommandKind::Fill, focused.focusBar, &p.focusBar) == 1, name + "one focus bar");
    require(countRect(list, UiCommandKind::Border, focused.focusBorder, &p.focusBorder) == 1, name + "one focus border");
    int borders = 0;
    for (const UiCommand& c : list.commands())
        if (c.kind == UiCommandKind::Border && sameColour(c.colour, colourOf(p.focusBorder))) {
            ++borders;
            require(c.thickness == static_cast<float>(layout.scale.border), name + "focus border thickness");
        }
    require(borders == 1, name + "focus border only on the focused row");

    require(countRect(list, UiCommandKind::Fill, layout.rows[0].track, &p.sliderTrack) == 1, name + "slider track");
    require(countRect(list, UiCommandKind::Fill, layout.rows[0].thumb) == 1, name + "slider thumb");
    require(countText(list, "70") == 1, name + "slider number");
    require(countOf(list, UiCommandKind::Line) == 4 + 4 + 2 + 4, name + "chevrons: two selectors, one toggle, one dropdown");
    for (std::size_t i = 0; i < screen.rows.size(); ++i) {
        require(countText(list, screen.rows[i].label) == 1, name + "row label " + screen.rows[i].label);
        require(countText(list, screen.rows[i].value) == 1, name + "row value " + screen.rows[i].value);
    }
    const UiCommand* disabledLabel = nullptr;
    const UiCommand* enabledLabel = nullptr;
    const UiCommand* focusedLabel = nullptr;
    for (const UiCommand& c : list.commands()) {
        if (c.kind != UiCommandKind::Text) continue;
        if (c.text == "Setting 5") disabledLabel = &c;
        if (c.text == "Setting 1") enabledLabel = &c;
        if (c.text == "Setting 2") focusedLabel = &c;
    }
    require(disabledLabel && sameColour(disabledLabel->colour, colourOf(p.textDisabled)), name + "disabled row in the disabled colour");
    require(enabledLabel && sameColour(enabledLabel->colour, colourOf(p.text)), name + "enabled row in the text colour");
    require(focusedLabel && sameColour(focusedLabel->colour, colourOf(p.focusText)), name + "focused label in the focus text colour");

    require(countText(list, "Back") == 1, name + "back button text");
    for (const UiCommand& c : list.commands())
        if (c.kind == UiCommandKind::Text && c.text == "Back") require(clipRect(c) == layout.backButton, name + "back text clipped to the bottom-left button");
    require(layout.backButton.x < layout.hintText.x && layout.backButton.y == layout.hintBar.y, name + "back button bottom-left");
    require(countText(list, defaultHintText(InputDevice::Keyboard)) == 1, name + "keyboard hint");
    require(countText(list, chrome.helpText) == 1, name + "help line");
    require(layout.scroll.maxOffset == 0 && countRect(list, UiCommandKind::Fill, layout.scroll.track) == 0, name + "no scrollbar without overflow");

    for (const UiCommand& c : list.commands()) {
        if (!c.clipped || c.kind == UiCommandKind::Text) continue;
        if (!insideRect(clipRect(c), layout.viewport)) continue;
        require(!overlaps(clipRect(c), layout.hintBar), name + "row commands do not reach the hint bar");
    }
    checkBalancedAndClipped(list, screen, layout, name);

    for (const UiCommand& c : list.commands()) {
        if (c.kind != UiCommandKind::Text) continue;
        const Color* allowed[] = {&p.text, &p.textDisabled, &p.focusText};
        const bool known = std::any_of(std::begin(allowed), std::end(allowed), [&](const Color* a) { return sameColour(c.colour, colourOf(*a)); });
        require(known, name + "text uses a palette pair: " + c.text);
        require(c.pixelSize == layout.scale.textPixels, name + "text size from the layout");
    }

    screen.focusOnTabs = true;
    const UiDrawList tabsFocused = paint(screen, chrome, &layout);
    require(countRect(tabsFocused, UiCommandKind::Border, layout.tabs[0].rect, &p.focusBorder) == 1, name + "focused tab bordered");
    require(countRect(tabsFocused, UiCommandKind::Fill, layout.tabs[0].rect, &p.focusBar) == 1, name + "focused tab filled");
    require(countRect(tabsFocused, UiCommandKind::Border, layout.rows[2].focusBorder) == 0, name + "no row focus while the tabs have it");
}

void checkDevices(const Size& size) {
    const std::string name = std::to_string(size.height) + "p ";
    const Screen screen = sampleScreen(size.width, size.height);
    const InputDevice devices[] = {InputDevice::Keyboard, InputDevice::Mouse, InputDevice::Pad};
    std::vector<std::string> seen;
    for (InputDevice device : devices) {
        MenuChrome chrome = sampleChrome();
        chrome.device = device;
        const UiDrawList list = paint(screen, chrome);
        const std::string hint = defaultHintText(device);
        require(!hint.empty() && countText(list, hint) == 1, name + "hint of the device");
        require(std::find(seen.begin(), seen.end(), hint) == seen.end(), name + "hints differ by device");
        seen.push_back(hint);
    }
    require(defaultHintText(InputDevice::Keyboard) == "Up/Down move  Left/Right change  Enter select  Esc back", name + "keyboard hint text");
    require(defaultHintText(InputDevice::Pad) == "D-pad move  A select  B back  LB/RB tabs", name + "pad hint text");
    MenuChrome custom = sampleChrome();
    custom.hintText = "Custom hint";
    const UiDrawList list = paint(screen, custom);
    require(countText(list, "Custom hint") == 1 && countText(list, defaultHintText(InputDevice::Keyboard)) == 0, name + "own hint text replaces the default");
}

void checkDropdown(const Size& size) {
    const std::string name = std::to_string(size.height) + "p ";
    Screen screen = sampleScreen(size.width, size.height);
    screen.focusRow = 3;
    screen.openDropdownRow = 3;
    screen.dropdownItemCount = 12;
    MenuChrome chrome = sampleChrome();
    for (int i = 0; i < 12; ++i) chrome.dropdownItems.push_back("Mode " + std::to_string(i));
    chrome.dropdownFocus = 4;
    chrome.dropdownSelected = 1;
    UiLayout layout;
    const UiDrawList list = paint(screen, chrome, &layout);
    const Palette& p = chrome.palette;
    require(layout.dropdown.open && !layout.dropdown.items.empty(), name + "dropdown is open");
    require(countRect(list, UiCommandKind::Fill, layout.dropdown.popup, &p.control) == 1, name + "popup fill");
    require(countRect(list, UiCommandKind::Border, layout.dropdown.popup, &p.focusBorder) == 1, name + "popup border");
    for (std::size_t i = 0; i < layout.dropdown.items.size(); ++i) {
        const int index = layout.dropdown.firstItem + static_cast<int>(i);
        require(countText(list, "Mode " + std::to_string(index)) == 1, name + "popup item text");
        require(countRect(list, UiCommandKind::Fill, layout.dropdown.items[i], &p.focusBar) == (index == 4 ? 1 : 0), name + "only the focused item has the bar");
        require(insideRect(layout.dropdown.items[i], layout.dropdown.popup), name + "item inside popup");
    }
    require(insideRect(layout.dropdown.popup, qol::Rect{0, 0, size.width, size.height}), name + "popup inside the window");
    require(!overlaps(layout.dropdown.popup, layout.hintBar), name + "popup keeps off the hint bar");
    int popupBorders = 0;
    for (const UiCommand& c : list.commands())
        if (c.kind == UiCommandKind::Border && sameColour(c.colour, colourOf(p.focusBorder)) && !c.clipped) ++popupBorders;
    require(popupBorders >= 1, name + "popup border not clipped to the rows");
    const auto& commands = list.commands();
    std::size_t popupAt = 0;
    std::size_t lastRowText = 0;
    for (std::size_t i = 0; i < commands.size(); ++i) {
        if (commands[i].kind == UiCommandKind::Fill && sameRect(commands[i], layout.dropdown.popup)) popupAt = i;
        if (commands[i].kind == UiCommandKind::Text && commands[i].text == "Setting 4") lastRowText = i;
    }
    require(popupAt > lastRowText, name + "popup drawn after the rows");
    checkBalancedAndClipped(list, screen, layout, name);
}

void checkDialog(const Size& size) {
    const std::string name = std::to_string(size.height) + "p ";
    const Screen screen = sampleScreen(size.width, size.height);
    MenuChrome chrome = sampleChrome();
    MenuDialog dialog;
    dialog.title = "Keep these settings?";
    dialog.body = "The window mode or the resolution changed. The previous settings return when the time runs out, unless you keep the new ones.";
    dialog.buttons = {"Keep", "Revert"};
    dialog.focusedButton = 0;
    dialog.countdownText = "Reverting in 15 s";
    chrome.dialog = dialog;
    UiLayout layout;
    const UiDrawList list = paint(screen, chrome, &layout);
    const Palette& p = chrome.palette;
    const DialogLayout d = computeDialogLayout(dialog, layout.scale, fakeWidth, fakeLine);

    require(countOf(list, UiCommandKind::Dim) == 2, name + "second dim under the dialog");
    require(d.box.width > 0 && insideRect(d.box, qol::Rect{0, 0, size.width, size.height}), name + "dialog box inside the window");
    require(std::abs((d.box.x + d.box.width / 2) - size.width / 2) <= 1 && std::abs((d.box.y + d.box.height / 2) - size.height / 2) <= 1, name + "dialog centred");
    require(countRect(list, UiCommandKind::Fill, d.box, &p.panel) == 1, name + "dialog fill");
    require(countRect(list, UiCommandKind::Border, d.box, &p.focusBorder) == 1, name + "dialog border");
    require(countText(list, dialog.title) == 1 && countText(list, dialog.countdownText) == 1, name + "title and countdown");
    require(d.bodyLines.size() >= 2, name + "long body wraps");
    std::string joined;
    for (std::size_t i = 0; i < d.bodyLines.size(); ++i) {
        joined += (i ? " " : "") + d.bodyLines[i];
        require(countText(list, d.bodyLines[i]) == 1, name + "body line drawn");
        require(fakeWidth(d.bodyLines[i], layout.scale.textPixels) <= static_cast<float>(d.title.width), name + "body line fits the box");
    }
    require(joined == dialog.body, name + "wrapped body keeps its words");
    require(d.buttons.size() == 2, name + "two buttons");
    require(countRect(list, UiCommandKind::Fill, d.buttons[0], &p.focusBar) == 1, name + "focused button has the bar");
    require(countRect(list, UiCommandKind::Border, d.buttons[0], &p.focusBorder) == 1, name + "focused button has the border");
    require(countRect(list, UiCommandKind::Fill, d.buttons[1], &p.control) == 1, name + "other button plain");
    require(countRect(list, UiCommandKind::Border, d.buttons[1]) == 0, name + "other button without border");
    require(!overlaps(d.buttons[0], d.buttons[1]), name + "buttons apart");
    std::vector<qol::Rect> parts{d.title, d.countdown, d.buttons[0], d.buttons[1]};
    for (const qol::Rect& r : d.bodyRects) parts.push_back(r);
    for (std::size_t i = 0; i < parts.size(); ++i) {
        require(insideRect(parts[i], d.box), name + "dialog part inside the box");
        for (std::size_t j = i + 1; j < parts.size(); ++j) require(!overlaps(parts[i], parts[j]), name + "dialog parts do not overlap");
    }
    for (std::size_t i = 0; i < d.buttons.size(); ++i) {
        const int cx = d.buttons[i].x + d.buttons[i].width / 2;
        const int cy = d.buttons[i].y + d.buttons[i].height / 2;
        require(hitDialogButton(d, cx, cy) == static_cast<int>(i), name + "button hit");
        require(d.buttons[i].height >= layout.scale.minHit, name + "button at least the minimum hit size");
    }
    require(hitDialogButton(d, d.box.x, d.box.y) == -1, name + "no hit outside the buttons");

    chrome.dialog->focusedButton = 1;
    const UiDrawList moved = paint(screen, chrome);
    require(countRect(moved, UiCommandKind::Fill, d.buttons[1], &p.focusBar) == 1 && countRect(moved, UiCommandKind::Fill, d.buttons[0], &p.control) == 1, name + "focus moves to the other button");

    const std::size_t dialogDim = static_cast<std::size_t>(std::find_if(list.commands().begin() + 1, list.commands().end(), [](const UiCommand& c) { return c.kind == UiCommandKind::Dim; }) - list.commands().begin());
    require(dialogDim > 10 && dialogDim < list.commands().size() - 5, name + "dialog dim sits between the menu and the dialog");

    MenuDialog bare;
    bare.title = "Press a key...";
    const DialogLayout plain = computeDialogLayout(bare, layout.scale, fakeWidth, fakeLine);
    require(plain.buttons.empty() && plain.bodyLines.empty() && plain.countdown.height == 0, name + "dialog without buttons, body or countdown");
    require(plain.box.height < d.box.height, name + "smaller dialog when nothing else is shown");
    chrome.dialog = bare;
    const UiDrawList prompt = paint(screen, chrome);
    require(countText(prompt, "Press a key...") == 1 && countOf(prompt, UiCommandKind::Dim) == 2, name + "prompt dialog");
    checkBalancedAndClipped(list, screen, layout, name);
}

void checkScroll(const Size& size) {
    const std::string name = std::to_string(size.height) + "p ";
    Screen screen = sampleScreen(size.width, size.height, 40);
    screen.focusRow = 12;
    screen.scrollOffset = 11 * computeLayout(screen, fakeWidth).scale.rowHeight + 7;
    MenuChrome chrome = sampleChrome();
    UiLayout layout;
    const UiDrawList list = paint(screen, chrome, &layout);
    const Palette& p = chrome.palette;
    require(layout.scroll.maxOffset > 0, name + "overflowing rows scroll");
    require(countRect(list, UiCommandKind::Fill, layout.scroll.track, &p.control) == 1, name + "scrollbar track");
    require(countRect(list, UiCommandKind::Fill, layout.scroll.thumb, &p.sliderTrack) == 1, name + "scrollbar thumb");
    require(insideRect(layout.scroll.thumb, layout.scroll.track), name + "thumb inside track");
    int visible = 0;
    for (std::size_t i = 0; i < layout.rows.size(); ++i) {
        visible += layout.rows[i].visible;
        const int drawn = countText(list, "Setting " + std::to_string(i));
        const int wanted = insideRect(layout.rows[i].rect, layout.viewport) ? 1 : 0;
        require(layout.rows[i].visible ? drawn >= wanted && drawn <= 1 : drawn == 0, name + "only visible rows are drawn");
    }
    require(visible > 0 && visible < 40, name + "some rows scrolled out");
    require(countText(list, "Game") == 1 && countText(list, defaultHintText(InputDevice::Keyboard)) == 1, name + "tab bar and hint bar stay");
    for (const UiCommand& c : list.commands()) {
        if (c.kind == UiCommandKind::Text && c.text.rfind("Setting ", 0) == 0)
            require(insideRect(clipRect(c), layout.viewport), name + "row text clipped to the viewport");
    }
    checkBalancedAndClipped(list, screen, layout, name);
}

void checkBatch(const Size& size) {
    const std::string name = std::to_string(size.height) + "p ";
    Screen screen = sampleScreen(size.width, size.height);
    screen.openDropdownRow = 3;
    screen.dropdownItemCount = 12;
    MenuChrome chrome = sampleChrome();
    chrome.dialog = MenuDialog{"Title", "Body", {"OK"}, 0, ""};
    const UiDrawList list = paint(screen, chrome);
    const UiBatch batch = chase::render::buildUiBatch(list, size.width, size.height);
    int texts = 0;
    for (const UiCommand& c : list.commands()) texts += c.kind == UiCommandKind::Text;
    require(batch.texts.size() == static_cast<std::size_t>(texts), name + "every text reaches the batch");
    require(!batch.vertices.empty() && !batch.runs.empty(), name + "shapes batched");
    bool windowed = true;
    for (const chase::render::UiRun& run : batch.runs)
        if (run.clipped && (run.clip.x < 0 || run.clip.y < 0 || run.clip.x + run.clip.width > size.width || run.clip.y + run.clip.height > size.height)) windowed = false;
    for (const chase::render::UiTextRun& run : batch.texts)
        if (run.clipped && (run.clip.x < 0 || run.clip.y < 0 || run.clip.x + run.clip.width > size.width || run.clip.y + run.clip.height > size.height)) windowed = false;
    require(windowed, name + "scissor runs inside the window");
    std::size_t last = 0;
    bool ordered = true;
    for (const chase::render::UiTextRun& run : batch.texts) {
        if (run.runsBefore < last) ordered = false;
        last = run.runsBefore;
    }
    require(ordered, name + "texts keep their order against the shape runs");
}

void checkBundledFont(const Size& size) {
    const std::string name = std::to_string(size.height) + "p bundled font ";
    qol::Settings settings;
    QolPage page(settings);
    Screen screen;
    screen.windowWidth = size.width;
    screen.windowHeight = size.height;
    screen.tabs = {"Game", "Display", "Controls", "Quality of life"};
    screen.selectedTab = 3;
    screen.rows = page.rows();
    screen.focusRow = 1;
    const TextMeasure measure = [](std::string_view t, float px) { return chase::render::uiTextWidth(t, px); };
    const UiLayout layout = computeLayout(screen, measure);
    for (std::size_t i = 0; i < layout.tabs.size(); ++i) require(layout.tabs[i].labelFits, name + "tab label fits");
    for (std::size_t i = 0; i < layout.rows.size(); ++i) {
        require(layout.rows[i].labelFits, name + "row label fits: " + screen.rows[i].label);
        require(layout.rows[i].valueFits, name + "row value fits: " + screen.rows[i].value);
        const float help = measure(page.help(static_cast<int>(i)).text, layout.scale.textPixels);
        require(help <= static_cast<float>(layout.helpLine.width - layout.scale.padding), name + "help line fits: " + page.help(static_cast<int>(i)).text);
    }
    for (InputDevice device : {InputDevice::Keyboard, InputDevice::Mouse, InputDevice::Pad})
        require(measure(defaultHintText(device), layout.scale.textPixels) <= static_cast<float>(layout.hintText.width), name + "hint text fits");
    MenuChrome chrome;
    chrome.helpText = page.help(1).text;
    UiDrawList list;
    paintMenu(list, screen, layout, chrome);
    require(countText(list, chrome.helpText) == 1 && countText(list, "Quality of life") == 1, name + "default measure path paints");
    checkBalancedAndClipped(list, screen, layout, name);
}

void checkContrast() {
    const Palette p;
    for (const ContrastPair& pair : contrastPairs(p)) require(contrastRatio(pair.foreground, pair.background) >= minContrast, std::string("contrast ") + pair.name);
    require(contrastRatio(p.textDisabled, p.control) >= minContrast, "disabled text on the control box");
    require(contrastRatio(p.text, p.panel) >= minContrast, "text on the panel");
    require(contrastRatio(p.focusText, p.focusBar) >= minContrast, "focus text on the bar");
}

}

int main() {
    checkContrast();
    for (const Size& size : sizes) {
        checkChrome(size);
        checkDevices(size);
        checkDropdown(size);
        checkDialog(size);
        checkScroll(size);
        checkBatch(size);
        checkBundledFont(size);
    }
    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    return 0;
}
