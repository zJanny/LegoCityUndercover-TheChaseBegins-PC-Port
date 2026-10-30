#include "render/ui_draw.hpp"
#include <algorithm>
#include <cmath>

namespace chase::render {
namespace {

struct Point {
    float x, y;
};

constexpr float pi = 3.14159265358979f;

std::vector<Point> outline(const UiRect& rect, float r, int segments) {
    const float centres[4][2] = {{rect.x + r, rect.y + r},
                                 {rect.x + rect.width - r, rect.y + r},
                                 {rect.x + rect.width - r, rect.y + rect.height - r},
                                 {rect.x + r, rect.y + rect.height - r}};
    const float starts[4] = {180, 270, 0, 90};
    std::vector<Point> points;
    for (int corner = 0; corner < 4; ++corner) {
        for (int i = 0; i <= segments; ++i) {
            const float angle = (starts[corner] + 90.0f * float(i) / float(segments)) * pi / 180.0f;
            points.push_back({centres[corner][0] + r * std::cos(angle), centres[corner][1] + r * std::sin(angle)});
        }
    }
    return points;
}

void triangle(std::vector<UiVertex>& out, const Point& a, const Point& b, const Point& c, const UiColour& colour) {
    for (const Point& p : {a, b, c}) out.push_back({p.x, p.y, colour.r, colour.g, colour.b, colour.a});
}

void fillRect(std::vector<UiVertex>& out, const UiRect& rect, float radius, const UiColour& colour) {
    const float r = std::max(0.0f, std::min(radius, std::min(rect.width, rect.height) / 2));
    const std::vector<Point> points = outline(rect, r, r > 0 ? uiCornerSegments(r) : 1);
    const Point centre{rect.x + rect.width / 2, rect.y + rect.height / 2};
    for (std::size_t i = 0; i < points.size(); ++i) triangle(out, centre, points[i], points[(i + 1) % points.size()], colour);
}

void borderRect(std::vector<UiVertex>& out, const UiCommand& command) {
    const UiRect& rect = command.rect;
    const float limit = std::min(rect.width, rect.height) / 2;
    const float t = std::max(0.0f, std::min(command.thickness, limit));
    const float r = std::max(0.0f, std::min(command.radius, limit));
    const int segments = r > 0 ? uiCornerSegments(r) : 1;
    const std::vector<Point> outer = outline(rect, r, segments);
    const std::vector<Point> inner = outline({rect.x + t, rect.y + t, rect.width - 2 * t, rect.height - 2 * t}, std::max(0.0f, r - t), segments);
    for (std::size_t i = 0; i < outer.size(); ++i) {
        const std::size_t j = (i + 1) % outer.size();
        triangle(out, outer[i], outer[j], inner[i], command.colour);
        triangle(out, outer[j], inner[j], inner[i], command.colour);
    }
}

void lineQuad(std::vector<UiVertex>& out, const UiCommand& command) {
    const float x0 = command.rect.x, y0 = command.rect.y;
    const float dx = command.x1 - x0, dy = command.y1 - y0;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length <= 0) return;
    const float nx = -dy / length * command.thickness / 2, ny = dx / length * command.thickness / 2;
    const Point a{x0 + nx, y0 + ny}, b{command.x1 + nx, command.y1 + ny}, c{command.x1 - nx, command.y1 - ny}, d{x0 - nx, y0 - ny};
    triangle(out, a, b, c, command.colour);
    triangle(out, a, c, d, command.colour);
}

bool sameClip(const UiClip& a, const UiClip& b) {
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

}

int uiCornerSegments(float radius) {
    return std::clamp(int(std::ceil(radius / 2.0f)), 2, 24);
}

UiClip intersectClips(const UiClip& a, const UiClip& b) {
    const int x0 = std::max(a.x, b.x), y0 = std::max(a.y, b.y);
    const int x1 = std::min(a.x + a.width, b.x + b.width), y1 = std::min(a.y + a.height, b.y + b.height);
    return {x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)};
}

void UiDrawList::add(UiCommand command) {
    if (!clips_.empty()) {
        command.clipped = true;
        command.clip = clips_.back();
    }
    commands_.push_back(std::move(command));
}

void UiDrawList::fill(const UiRect& rect, const UiColour& colour, float radius) {
    UiCommand command;
    command.kind = UiCommandKind::Fill;
    command.rect = rect;
    command.colour = colour;
    command.radius = radius;
    add(std::move(command));
}

void UiDrawList::border(const UiRect& rect, const UiColour& colour, float thickness, float radius) {
    UiCommand command;
    command.kind = UiCommandKind::Border;
    command.rect = rect;
    command.colour = colour;
    command.thickness = thickness;
    command.radius = radius;
    add(std::move(command));
}

void UiDrawList::line(float x0, float y0, float x1, float y1, float thickness, const UiColour& colour) {
    UiCommand command;
    command.kind = UiCommandKind::Line;
    command.rect = {x0, y0, 0, 0};
    command.x1 = x1;
    command.y1 = y1;
    command.thickness = thickness;
    command.colour = colour;
    add(std::move(command));
}

void UiDrawList::dim(const UiColour& colour) {
    UiCommand command;
    command.kind = UiCommandKind::Dim;
    command.colour = colour;
    add(std::move(command));
}

void UiDrawList::text(const std::string& text, float pixelSize, float x, float y, const UiColour& colour) {
    UiCommand command;
    command.kind = UiCommandKind::Text;
    command.text = text;
    command.pixelSize = pixelSize;
    command.rect = {x, y, 0, 0};
    command.colour = colour;
    add(std::move(command));
}

void UiDrawList::pushClip(const UiClip& clip) {
    clips_.push_back(clips_.empty() ? clip : intersectClips(clips_.back(), clip));
}

void UiDrawList::popClip() {
    if (!clips_.empty()) clips_.pop_back();
}

void UiDrawList::clear() {
    commands_.clear();
    clips_.clear();
}

UiBatch buildUiBatch(const UiDrawList& list, int windowWidth, int windowHeight) {
    UiBatch batch;
    const UiClip window{0, 0, windowWidth, windowHeight};
    bool textSinceRun = false;
    for (const UiCommand& command : list.commands()) {
        const UiClip clip = command.clipped ? intersectClips(window, command.clip) : window;
        if (clip.width <= 0 || clip.height <= 0) continue;
        if (command.kind == UiCommandKind::Text) {
            batch.texts.push_back({command.text, command.pixelSize, command.rect.x, command.rect.y, command.colour, command.clipped, clip, batch.runs.size()});
            textSinceRun = true;
            continue;
        }
        std::vector<UiVertex> vertices;
        const bool visible = command.rect.width > 0 && command.rect.height > 0;
        if (command.kind == UiCommandKind::Fill && visible) fillRect(vertices, command.rect, command.radius, command.colour);
        else if (command.kind == UiCommandKind::Border && visible && command.thickness > 0) borderRect(vertices, command);
        else if (command.kind == UiCommandKind::Line) lineQuad(vertices, command);
        else if (command.kind == UiCommandKind::Dim) fillRect(vertices, {0, 0, float(windowWidth), float(windowHeight)}, 0, command.colour);
        if (vertices.empty()) continue;
        const bool merge = !textSinceRun && !batch.runs.empty() && batch.runs.back().clipped == command.clipped &&
                           (!command.clipped || sameClip(batch.runs.back().clip, clip));
        textSinceRun = false;
        if (!merge) batch.runs.push_back({command.clipped, clip, batch.vertices.size(), 0});
        batch.runs.back().count += vertices.size();
        batch.vertices.insert(batch.vertices.end(), vertices.begin(), vertices.end());
    }
    return batch;
}

}
