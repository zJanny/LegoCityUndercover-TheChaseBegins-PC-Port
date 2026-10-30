#pragma once
#include "render/ui_text_layout.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace chase::render {

struct UiColour {
    float r = 0, g = 0, b = 0, a = 1;
};

// Window pixels with y counting down from the top of the window.
struct UiRect {
    float x = 0, y = 0, width = 0, height = 0;
};

enum class UiCommandKind { Fill, Border, Line, Dim, Text };

struct UiCommand {
    UiCommandKind kind = UiCommandKind::Fill;
    UiRect rect;
    float radius = 0;
    float thickness = 1;
    float x1 = 0, y1 = 0;
    UiColour colour;
    std::string text;
    float pixelSize = 0;
    bool clipped = false;
    UiClip clip;
};

class UiDrawList {
public:
    void fill(const UiRect& rect, const UiColour& colour, float radius = 0);
    void border(const UiRect& rect, const UiColour& colour, float thickness, float radius = 0);
    void line(float x0, float y0, float x1, float y1, float thickness, const UiColour& colour);
    void dim(const UiColour& colour);
    void text(const std::string& text, float pixelSize, float x, float y, const UiColour& colour);
    void pushClip(const UiClip& clip);
    void popClip();
    const std::vector<UiCommand>& commands() const { return commands_; }
    void clear();

private:
    void add(UiCommand command);
    std::vector<UiCommand> commands_;
    std::vector<UiClip> clips_;
};

struct UiVertex {
    float x, y;
    float r, g, b, a;
};

struct UiRun {
    bool clipped = false;
    UiClip clip;
    std::size_t first = 0, count = 0;
};

struct UiTextRun {
    std::string text;
    float pixelSize = 0;
    float x = 0, y = 0;
    UiColour colour;
    bool clipped = false;
    UiClip clip;
    std::size_t runsBefore = 0;
};

struct UiBatch {
    std::vector<UiVertex> vertices;
    std::vector<UiRun> runs;
    std::vector<UiTextRun> texts;
};

int uiCornerSegments(float radius);
UiClip intersectClips(const UiClip& a, const UiClip& b);
UiBatch buildUiBatch(const UiDrawList& list, int windowWidth, int windowHeight);

class UiDrawImpl;

class UiDraw {
public:
    UiDraw();
    ~UiDraw();
    UiDraw(const UiDraw&) = delete;
    UiDraw& operator=(const UiDraw&) = delete;

    void draw(const UiBatch& batch, int windowWidth, int windowHeight);

private:
    std::unique_ptr<UiDrawImpl> impl_;
};

}
