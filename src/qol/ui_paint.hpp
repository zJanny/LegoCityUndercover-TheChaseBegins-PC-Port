#pragma once

#include "qol/ui_layout.hpp"
#include "render/ui_draw.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace qol::ui {

enum class InputDevice { Keyboard, Mouse, Pad };

std::string defaultHintText(InputDevice device);

struct MenuDialog {
    std::string title;
    std::string body;
    std::vector<std::string> buttons;
    int focusedButton = 0;
    std::string countdownText;
};

using LineHeight = std::function<float(float)>;

struct MenuChrome {
    Palette palette;
    std::string helpText;
    InputDevice device = InputDevice::Keyboard;
    std::string hintText;
    float dimAlpha = 0.72f;
    float dialogDimAlpha = 0.5f;
    std::vector<std::string> dropdownItems;
    int dropdownFocus = 0;
    int dropdownSelected = -1;
    std::optional<MenuDialog> dialog;
    TextMeasure measure;
    LineHeight lineHeight;
};

struct DialogLayout {
    Rect box;
    Rect title;
    std::vector<std::string> bodyLines;
    std::vector<Rect> bodyRects;
    Rect countdown;
    std::vector<Rect> buttons;
};

DialogLayout computeDialogLayout(const MenuDialog& dialog, const Scale& scale, const TextMeasure& measure, const LineHeight& lineHeight);
int hitDialogButton(const DialogLayout& layout, int x, int y);

void paintMenu(chase::render::UiDrawList& list, const Screen& screen, const UiLayout& layout, const MenuChrome& chrome);

}
