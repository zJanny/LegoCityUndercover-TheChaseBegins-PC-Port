#pragma once
// Text of the bundled UI font drawn at window resolution. draw() queues; flush() paints the queue. Plain types only.
#include "render/ui_text_layout.hpp"
#include <memory>
#include <string_view>

namespace chase::render {

class UiTextImpl;

class UiText {
public:
    UiText();
    ~UiText();
    UiText(const UiText&) = delete;
    UiText& operator=(const UiText&) = delete;

    // (x, y) is the top left of the line box in window pixels (y down); pixelSize is the em size in window pixels.
    void draw(std::string_view utf8, float pixelSize, float x, float y, UiColour colour, const UiClip* clip = nullptr);
    void flush(int windowWidth, int windowHeight);

private:
    std::unique_ptr<UiTextImpl> impl_;
};

}
