#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "qol/view_layout.hpp"

namespace qol::ui {

using TextMeasure = std::function<float(std::string_view, float)>;

constexpr double textHeightFraction = 0.025;
constexpr double rowHeightFraction = 0.055;
constexpr int minHitPixelsAt1080 = 40;
constexpr int referenceHeight = 1080;
constexpr double minContrast = 4.5;

struct Proportions {
    double columnWidthOfHeight = 1.3;
    double columnWidthOfWidthMax = 0.9;
    double controlShareOfColumn = 0.45;
    double marginOfRow = 0.5;
    double paddingOfText = 0.6;
    double sliderTrackOfRow = 0.12;
    double sliderThumbOfRow = 0.5;
    double underlineOfHeight = 0.004;
    double borderOfHeight = 0.003;
    double scrollbarOfRow = 0.12;
};

struct Scale {
    int windowWidth = 0;
    int windowHeight = 0;
    float textPixels = 0;
    int rowHeight = 0;
    int minHit = 0;
    int margin = 0;
    int padding = 0;
    int underline = 0;
    int border = 0;
    int tabBarHeight = 0;
    int helpLineHeight = 0;
    int hintBarHeight = 0;
    int columnX = 0;
    int columnWidth = 0;
    int controlWidth = 0;
};

Scale computeScale(int windowWidth, int windowHeight, const Proportions& proportions = {});

enum class RowKind { Slider, Selector, Toggle, Button, Dropdown };

struct RowSpec {
    std::string label;
    RowKind kind = RowKind::Button;
    std::string value;
    double fraction = 0;
    bool enabled = true;
};

struct Screen {
    int windowWidth = 0;
    int windowHeight = 0;
    std::vector<std::string> tabs;
    int selectedTab = 0;
    std::vector<RowSpec> rows;
    int focusRow = -1;
    bool focusOnTabs = false;
    int scrollOffset = 0;
    int openDropdownRow = -1;
    int dropdownItemCount = 0;
    int dropdownFirstItem = 0;
    std::string backText = "Back";
};

struct TabLayout {
    Rect rect;
    Rect underline;
    bool selected = false;
    bool focused = false;
    bool labelFits = true;
};

struct RowLayout {
    Rect rect;
    Rect label;
    Rect control;
    Rect leftArrow;
    Rect rightArrow;
    Rect valueArea;
    Rect track;
    Rect thumb;
    Rect numberBox;
    Rect focusBar;
    Rect focusBorder;
    bool visible = false;
    bool focused = false;
    bool labelFits = true;
    bool valueFits = true;
};

struct ScrollLayout {
    int offset = 0;
    int maxOffset = 0;
    int contentHeight = 0;
    Rect track;
    Rect thumb;
};

struct DropdownLayout {
    bool open = false;
    bool flippedUp = false;
    Rect popup;
    std::vector<Rect> items;
    int firstItem = 0;
};

struct UiLayout {
    Scale scale;
    Rect tabBar;
    std::vector<TabLayout> tabs;
    Rect viewport;
    std::vector<RowLayout> rows;
    ScrollLayout scroll;
    DropdownLayout dropdown;
    Rect helpLine;
    Rect hintBar;
    Rect backButton;
    Rect hintText;
};

UiLayout computeLayout(const Screen& screen, const TextMeasure& measure, const Proportions& proportions = {});

int scrollToShowRow(const Screen& screen, int row, const Proportions& proportions = {});
int clampScroll(const Screen& screen, int offset, const Proportions& proportions = {});
int wheelScroll(const Screen& screen, int notches, const Proportions& proportions = {});

enum class Target { None, Tab, Back, Row, DropdownItem, Scrollbar };
enum class Part { None, Body, LeftArrow, RightArrow, Value, Track, Number };

struct Hit {
    Target target = Target::None;
    Part part = Part::None;
    int index = -1;
};

bool contains(const Rect& rect, int x, int y);
Hit hitTest(const UiLayout& layout, const Screen& screen, int x, int y);
double sliderFractionAt(const RowLayout& row, int x, int steps);

struct Color {
    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;
};

double contrastRatio(Color a, Color b);

struct Palette {
    Color panel{0x16, 0x18, 0x1D};
    Color text{0xF2, 0xF2, 0xF2};
    Color textDisabled{0xA6, 0xAA, 0xB3};
    Color focusBar{0x2F, 0x5F, 0xA8};
    Color focusText{0xFF, 0xFF, 0xFF};
    Color focusBorder{0xFF, 0xFF, 0xFF};
    Color control{0x2A, 0x2E, 0x37};
    Color sliderTrack{0x8A, 0x92, 0xA0};
    Color sliderFill{0x7F, 0xB2, 0xFF};
    Color tabUnderline{0x7F, 0xB2, 0xFF};
};

struct ContrastPair {
    const char* name;
    Color foreground;
    Color background;
};

std::vector<ContrastPair> contrastPairs(const Palette& palette);

}
