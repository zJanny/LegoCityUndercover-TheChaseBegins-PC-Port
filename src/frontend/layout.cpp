#include "frontend/layout.hpp"
#include <algorithm>
#include <cmath>

namespace chase::frontend {

namespace {

std::string lowerCase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

float bezier(float a, float b, float t) {
    const float u = 1 - t;
    return 3 * u * u * t * a + 3 * u * t * t * b + t * t * t;
}

float easedFraction(const float spline[4], float fraction) {
    float low = 0, high = 1;
    for (int i = 0; i < 40; ++i) {
        const float mid = (low + high) / 2;
        if (bezier(spline[0], spline[2], mid) < fraction) low = mid;
        else high = mid;
    }
    return bezier(spline[1], spline[3], (low + high) / 2);
}

struct Pose {
    float translateX = 0, translateY = 0, scaleX = 1, scaleY = 1, opacity = 1;
    bool hasOpacity = false;
};

Pose poseOf(const assets::UiLayout* layout, const std::string& storyboard, float time, const std::string& target) {
    Pose pose;
    if (!layout || storyboard.empty()) return pose;
    for (const auto& board : layout->storyboards) {
        if (board.name != storyboard) continue;
        for (const auto& track : board.tracks) {
            if (track.target != target) continue;
            const float value = evaluateTrack(track, time);
            switch (track.property) {
            case 1: pose.translateX += value; break;
            case 2: pose.translateY += value; break;
            case 3: pose.scaleX *= value; break;
            case 4: pose.scaleY *= value; break;
            case 6:
                pose.opacity = value;
                pose.hasOpacity = true;
                break;
            default: break;
            }
        }
    }
    return pose;
}

struct Frame {
    float x, y, scaleX, scaleY, opacity;
};

class Drawer {
public:
    Drawer(Canvas& canvas, Resources& resources, const LayoutScene& scene) : canvas_(canvas), resources_(resources), scene_(scene) {}

    void layout(const std::string& path, Frame frame, const std::string& storyboard, const std::string& instance,
                float time) {
        const assets::UiLayout& parsed = resources_.layout(path);
        const assets::UiLayout* previous = current_;
        const std::string previousPath = currentPath_, previousBoard = storyboard_, previousInstance = instance_;
        const float previousTime = time_;
        time_ = time;
        current_ = &parsed;
        currentPath_ = path;
        storyboard_ = storyboard;
        instance_ = instance;
        element(parsed.root, frame);
        current_ = previous;
        currentPath_ = previousPath;
        storyboard_ = previousBoard;
        instance_ = previousInstance;
        time_ = previousTime;
    }

private:
    const std::string* find(const std::map<std::string, std::string>& map, const std::string& name) const {
        if (!instance_.empty()) {
            const auto scoped = map.find(instance_ + "/" + name);
            if (scoped != map.end()) return &scoped->second;
        }
        const auto plain = map.find(name);
        return plain == map.end() ? nullptr : &plain->second;
    }

    void element(const assets::UiElement& e, const Frame& parent) {
        if (scene_.hidden.count(e.name) || (!instance_.empty() && scene_.hidden.count(instance_ + "/" + e.name))) return;
        const Pose pose = poseOf(current_, storyboard_, time_, e.name);
        float width = e.size[0], height = e.size[1];
        const assets::Image* image = nullptr;
        if (e.type == assets::UiElementType::Image) {
            const std::string* replaced = find(scene_.images, e.name);
            image = &resources_.image(replaced ? *replaced : layoutTexturePath(currentPath_, e.source));
            if (width <= 1e-6f) width = float(image->width);
            if (height <= 1e-6f) height = float(image->height);
        }
        const float scaleX = e.scale[0] * pose.scaleX, scaleY = e.scale[1] * pose.scaleY;
        const float localX = e.position[0] + pose.translateX, localY = e.position[1] + pose.translateY;
        const float pivotX = e.origin[0] * width, pivotY = e.origin[1] * height;
        const float boxX = localX + pivotX * (1 - scaleX), boxY = localY + pivotY * (1 - scaleY);
        Frame frame;
        frame.scaleX = parent.scaleX * scaleX;
        frame.scaleY = parent.scaleY * scaleY;
        frame.x = parent.x + parent.scaleX * boxX;
        frame.y = parent.y + parent.scaleY * boxY;
        frame.opacity = parent.opacity * (pose.hasOpacity ? pose.opacity : e.opacity);
        if (image) {
            const Colour tint{1, 1, 1, frame.opacity};
            canvas_.drawScaled(*image, {0, 0, image->width, image->height}, frame.x, frame.y, width * frame.scaleX,
                               height * frame.scaleY, tint);
        } else if (e.type == assets::UiElementType::Reference) {
            const auto board = scene_.referenceStoryboards.find(e.name);
            const auto referenceTime = scene_.referenceTimes.find(e.name);
            layout(layoutFolder(currentPath_) + lowerCase(e.source) + ".bxaml", frame,
                   board == scene_.referenceStoryboards.end() ? std::string() : board->second, e.name,
                   referenceTime == scene_.referenceTimes.end() ? scene_.time : referenceTime->second);
        }
        const TextSlot* slot = nullptr;
        if (!instance_.empty()) {
            const auto scoped = scene_.text.find(instance_ + "/" + e.name);
            if (scoped != scene_.text.end()) slot = &scoped->second;
        }
        if (!slot) {
            const auto plain = scene_.text.find(e.name);
            if (plain != scene_.text.end()) slot = &plain->second;
        }
        if (slot) text(*slot, e, frame, width, height);
        for (const auto& child : e.children) element(child, frame);
    }

    void text(const TextSlot& slot, const assets::UiElement& e, const Frame& frame, float width, float height) {
        const BitmapFont& font = resources_.font("frontendfont");
        const auto& extended = resources_.extendedCharacters();
        const int spacing = slot.spacing == TextSlot::kSpacingDefault ? scene_.textSpacing : slot.spacing;
        const std::vector<std::string> lines = slot.wrap ? wrapText(font, extended, slot.text, int(width) - 4, spacing)
                                                         : wrapText(font, extended, slot.text, 1 << 20, spacing);
        const int pitch = slot.pitch > 0 ? slot.pitch : int(font.font.lineHeight);
        const float boxWidth = width * frame.scaleX, boxHeight = height * frame.scaleY;
        float anchor = frame.x + boxWidth / 2;
        if (slot.align == Align::Left) anchor = frame.x;
        else if (slot.align == Align::Right) anchor = frame.x + boxWidth;
        const float blockHeight = float(pitch) * float(lines.size());
        float top = frame.y + (boxHeight - blockHeight) / 2 + float(slot.offsetY);
        Colour colour{e.colour[0] / 255.0f, e.colour[1] / 255.0f, e.colour[2] / 255.0f, e.colour[3] / 255.0f * frame.opacity};
        for (const auto& line : lines) {
            drawText(canvas_, font, extended, line, anchor, top + float(pitch - int(font.font.lineHeight)), slot.align, colour,
                     spacing, false);
            top += float(pitch);
        }
    }

    Canvas& canvas_;
    Resources& resources_;
    const LayoutScene& scene_;
    const assets::UiLayout* current_ = nullptr;
    std::string currentPath_, storyboard_, instance_;
    float time_ = 0;
};

}

std::string layoutFolder(const std::string& layoutPath) {
    const std::size_t slash = layoutPath.find_last_of('/');
    return slash == std::string::npos ? std::string() : layoutPath.substr(0, slash + 1);
}

std::string layoutTexturePath(const std::string& layoutPath, const std::string& source) {
    std::string name = lowerCase(source);
    std::replace(name.begin(), name.end(), '\\', '/');
    const auto ends = [&](const std::string& tail) {
        return name.size() >= tail.size() && name.compare(name.size() - tail.size(), tail.size(), tail) == 0;
    };
    if (ends(".png")) name.replace(name.size() - 4, 4, ".bpng");
    else if (ends(".tga")) name.replace(name.size() - 4, 4, ".btga");
    return layoutFolder(layoutPath) + name;
}

float evaluateTrack(const assets::UiTrack& track, float time) {
    const auto& keys = track.keyframes;
    if (keys.empty()) return 0;
    if (time <= keys.front().time) return keys.front().value;
    for (std::size_t i = 1; i < keys.size(); ++i) {
        if (time > keys[i].time) continue;
        const float span = keys[i].time - keys[i - 1].time;
        if (span <= 0) return keys[i].value;
        const float eased = easedFraction(keys[i].spline, (time - keys[i - 1].time) / span);
        return keys[i - 1].value + (keys[i].value - keys[i - 1].value) * eased;
    }
    return keys.back().value;
}

std::vector<std::string> wrapText(const BitmapFont& font, const std::vector<std::string>& extended,
                                  const std::string& text, int width, int spacing) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start <= text.size()) {
        std::size_t end = text.find('\n', start);
        if (end == std::string::npos) end = text.size();
        const std::string paragraph = text.substr(start, end - start);
        std::string line;
        std::size_t position = 0;
        while (position <= paragraph.size()) {
            std::size_t space = paragraph.find(' ', position);
            if (space == std::string::npos) space = paragraph.size();
            const std::string word = paragraph.substr(position, space - position);
            const std::string candidate = line.empty() ? word : line + " " + word;
            if (!line.empty() && textWidth(font, extended, candidate, spacing) > width) {
                lines.push_back(line);
                line = word;
            } else {
                line = candidate;
            }
            position = space + 1;
        }
        lines.push_back(line);
        start = end + 1;
    }
    return lines;
}

void drawLayout(Canvas& canvas, Resources& resources, const std::string& layoutPath, const LayoutScene& scene, float x,
                float y) {
    Drawer drawer(canvas, resources, scene);
    drawer.layout(layoutPath, {x, y, 1, 1, scene.opacity}, scene.storyboard, std::string(), scene.time);
}

}
