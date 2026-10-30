// Batching of the UI draw list (render/ui_draw.hpp), no OpenGL context. Values by hand computation, 20261030-qolui-B, 2026-10-30:
// an unrounded rectangle's outline is 8 points (one per corner twice) so its area is width * height; a corner with radius r and n
// segments per quarter has the inscribed polygon area 0.5 * r * r * n * sin(90 deg / n), so a rounded rectangle has
// width * height - 4 * (r * r - that); a border of thickness t has area w * h - (w - 2t) * (h - 2t); a line has area length * thickness.
#include "render/ui_draw.hpp"
#include <cmath>
#include <cstdio>

namespace {

using namespace chase::render;

int failures = 0;

void expect(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

bool near(double a, double b, double tolerance = 1e-3) { return std::fabs(a - b) <= tolerance; }

double area(const UiBatch& batch, std::size_t first, std::size_t count) {
    double sum = 0;
    for (std::size_t i = first; i + 2 < first + count + 0 && i + 2 < batch.vertices.size(); i += 3) {
        const UiVertex &a = batch.vertices[i], &b = batch.vertices[i + 1], &c = batch.vertices[i + 2];
        sum += std::fabs(double(a.x) * (double(b.y) - c.y) + double(b.x) * (double(c.y) - a.y) + double(c.x) * (double(a.y) - b.y)) / 2;
    }
    return sum;
}

double totalArea(const UiBatch& batch) { return area(batch, 0, batch.vertices.size()); }

bool inside(const UiBatch& batch, double x0, double y0, double x1, double y1) {
    for (const UiVertex& v : batch.vertices)
        if (v.x < x0 - 1e-4 || v.x > x1 + 1e-4 || v.y < y0 - 1e-4 || v.y > y1 + 1e-4) return false;
    return true;
}

double roundedArea(double w, double h, double r, int n = 0) {
    if (n == 0) n = uiCornerSegments(float(r));
    const double quarter = 0.5 * r * r * n * std::sin(1.5707963267948966 / n);
    return w * h - 4 * (r * r - quarter);
}

void fills() {
    UiDrawList list;
    list.fill({10, 20, 100, 50}, {1, 0, 0, 1});
    UiBatch batch = buildUiBatch(list, 200, 100);
    expect(batch.vertices.size() == 24 && batch.runs.size() == 1 && batch.runs[0].count == 24, "plain fill: 8 triangles in one run");
    expect(near(totalArea(batch), 5000), "plain fill area is width * height");
    expect(inside(batch, 10, 20, 110, 70), "plain fill stays in its rectangle");
    expect(batch.vertices[0].r == 1 && batch.vertices[0].a == 1, "fill vertices carry the colour");

    list.clear();
    list.fill({10, 20, 100, 50}, {0, 1, 0, 0.5f}, 10);
    batch = buildUiBatch(list, 200, 100);
    expect(uiCornerSegments(10) == 5, "radius 10 uses 5 segments per corner");
    expect(batch.vertices.size() == 3 * 24, "rounded fill: 24 outline points, 24 triangles");
    expect(near(totalArea(batch), roundedArea(100, 50, 10), 1e-2), "rounded fill area is the inscribed polygon area");
    expect(inside(batch, 10, 20, 110, 70), "rounded fill stays in its rectangle");

    list.clear();
    list.fill({0, 0, 20, 20}, {1, 1, 1, 1}, 100);
    batch = buildUiBatch(list, 200, 100);
    expect(near(totalArea(batch), roundedArea(20, 20, 10), 1e-2), "a radius larger than half the short side is clamped to it");

    list.clear();
    list.fill({0, 0, 0, 20}, {1, 1, 1, 1});
    list.fill({0, 0, 20, -5}, {1, 1, 1, 1});
    batch = buildUiBatch(list, 200, 100);
    expect(batch.vertices.empty() && batch.runs.empty(), "empty rectangles draw nothing");
}

void borders() {
    UiDrawList list;
    list.border({10, 20, 100, 50}, {1, 1, 1, 1}, 4);
    UiBatch batch = buildUiBatch(list, 200, 100);
    expect(near(totalArea(batch), 100 * 50 - 92 * 42), "square border area is w * h - (w - 2t) * (h - 2t)");
    expect(inside(batch, 10, 20, 110, 70), "border stays in its rectangle");

    list.clear();
    list.border({10, 20, 100, 50}, {1, 1, 1, 1}, 4, 10);
    batch = buildUiBatch(list, 200, 100);
    expect(near(totalArea(batch), roundedArea(100, 50, 10) - roundedArea(92, 42, 6, 5), 1e-2), "rounded border area is outer minus inner polygon (the inner corner uses the outer corner's 5 segments)");

    list.clear();
    list.border({0, 0, 10, 10}, {1, 1, 1, 1}, 50);
    batch = buildUiBatch(list, 200, 100);
    expect(near(totalArea(batch), 100) && inside(batch, 0, 0, 10, 10), "a border thicker than half the short side is clamped and fills the rectangle");

    list.clear();
    list.border({0, 0, 10, 10}, {1, 1, 1, 1}, 0);
    batch = buildUiBatch(list, 200, 100);
    expect(batch.vertices.empty(), "a border of thickness 0 draws nothing");
}

void lines() {
    UiDrawList list;
    list.line(10, 30, 60, 30, 2, {1, 1, 1, 1});
    UiBatch batch = buildUiBatch(list, 200, 100);
    expect(batch.vertices.size() == 6 && near(totalArea(batch), 100), "horizontal line: two triangles, length * thickness");
    expect(inside(batch, 10, 29, 60, 31), "horizontal line spans its thickness around the axis");

    list.clear();
    list.line(0, 0, 3, 4, 2, {1, 1, 1, 1});
    batch = buildUiBatch(list, 200, 100);
    expect(near(totalArea(batch), 10), "diagonal line area is length 5 * thickness 2");

    list.clear();
    list.line(5, 5, 5, 5, 2, {1, 1, 1, 1});
    batch = buildUiBatch(list, 200, 100);
    expect(batch.vertices.empty(), "a zero length line draws nothing");
}

void dimming() {
    UiDrawList list;
    list.dim({0, 0, 0, 0.6f});
    const UiBatch batch = buildUiBatch(list, 320, 200);
    expect(near(totalArea(batch), 320 * 200) && inside(batch, 0, 0, 320, 200), "dim covers the whole window");
    expect(near(batch.vertices[0].a, 0.6) && batch.runs.size() == 1 && !batch.runs[0].clipped, "dim keeps its alpha and needs no scissor");
}

void clips() {
    UiDrawList list;
    list.pushClip({50, 0, 100, 100});
    list.fill({0, 0, 200, 100}, {1, 1, 1, 1});
    list.fill({10, 10, 20, 20}, {1, 1, 1, 1});
    list.popClip();
    list.fill({0, 0, 10, 10}, {1, 1, 1, 1});
    UiBatch batch = buildUiBatch(list, 200, 100);
    expect(batch.runs.size() == 2, "equal clips merge into one run, an unclipped command starts another");
    expect(batch.runs[0].clipped && batch.runs[0].clip.x == 50 && batch.runs[0].clip.width == 100, "the run keeps the scissor rectangle");
    expect(!batch.runs[1].clipped && batch.runs[1].first == batch.runs[0].count, "the unclipped run follows the clipped run");

    list.clear();
    list.pushClip({0, 0, 100, 100});
    list.pushClip({60, 20, 100, 30});
    list.fill({0, 0, 200, 100}, {1, 1, 1, 1});
    list.popClip();
    list.fill({0, 0, 200, 100}, {1, 1, 1, 1});
    list.popClip();
    batch = buildUiBatch(list, 200, 100);
    expect(batch.runs.size() == 2 && batch.runs[0].clip.x == 60 && batch.runs[0].clip.width == 40 && batch.runs[0].clip.y == 20 && batch.runs[0].clip.height == 30,
           "nested clips intersect");
    expect(batch.runs[1].clip.x == 0 && batch.runs[1].clip.width == 100, "popping a clip restores the outer one");

    list.clear();
    list.pushClip({150, 0, 100, 100});
    list.fill({0, 0, 200, 100}, {1, 1, 1, 1});
    list.pushClip({-40, -40, 80, 80});
    list.fill({0, 0, 200, 100}, {1, 1, 1, 1});
    batch = buildUiBatch(list, 200, 100);
    expect(batch.runs.size() == 1 && batch.runs[0].clip.x == 150 && batch.runs[0].clip.width == 50, "a clip beyond the window is cut to the window");
    expect(batch.vertices.size() == 24, "a command whose clip is empty draws nothing");

    list.clear();
    list.pushClip({300, 300, 10, 10});
    list.fill({0, 0, 200, 100}, {1, 1, 1, 1});
    list.text("x", 12, 0, 0, {1, 1, 1, 1});
    batch = buildUiBatch(list, 200, 100);
    expect(batch.vertices.empty() && batch.runs.empty() && batch.texts.empty(), "a clip outside the window drops fills and texts");
}

void texts() {
    UiDrawList list;
    list.pushClip({10, 10, 50, 50});
    list.text("Quality of life", 24, 12, 34, {0.9f, 0.8f, 0.7f, 1});
    list.popClip();
    list.text("Back", 16, 5, 6, {1, 1, 1, 1});
    const UiBatch batch = buildUiBatch(list, 200, 100);
    expect(batch.vertices.empty() && batch.runs.empty(), "texts add no vertices");
    expect(batch.texts.size() == 2 && batch.texts[0].text == "Quality of life" && batch.texts[0].pixelSize == 24 && batch.texts[0].x == 12 &&
               batch.texts[0].y == 34 && batch.texts[0].clipped && batch.texts[0].clip.width == 50 && batch.texts[0].colour.g == 0.8f,
           "a text command is recorded with string, size, position, colour and clip");
    expect(!batch.texts[1].clipped && batch.texts[1].clip.width == 200, "an unclipped text carries the window as its clip");
}

void textOrder() {
    UiDrawList list;
    list.fill({0, 0, 10, 10}, {1, 0, 0, 1});
    list.text("a", 12, 0, 0, {1, 1, 1, 1});
    list.fill({0, 0, 10, 10}, {0, 0, 1, 1});
    list.text("b", 12, 0, 0, {1, 1, 1, 1});
    const UiBatch batch = buildUiBatch(list, 100, 100);
    expect(batch.runs.size() == 2, "a text between two fills of one clip splits the run");
    expect(batch.texts.size() == 2 && batch.texts[0].runsBefore == 1 && batch.texts[1].runsBefore == 2,
           "a text records how many runs precede it (20261030-qolui2-B, 2026-10-30, by hand)");
}

void order() {
    UiDrawList list;
    list.fill({0, 0, 10, 10}, {1, 0, 0, 1});
    list.dim({0, 0, 0, 0.5f});
    list.fill({0, 0, 10, 10}, {0, 0, 1, 1});
    const UiBatch batch = buildUiBatch(list, 100, 100);
    expect(batch.runs.size() == 1 && batch.vertices.front().r == 1 && batch.vertices[24].a == 0.5f && batch.vertices.back().b == 1,
           "commands keep their drawing order in the vertices");
}

}

int main() {
    fills();
    borders();
    lines();
    dimming();
    clips();
    texts();
    textOrder();
    order();
    return failures ? 1 : 0;
}
