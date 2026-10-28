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
    float translateX = 0, translateY = 0, scaleX = 1, scaleY = 1, rotation = 0, opacity = 1;
    bool hasOpacity = false, hasScaleX = false, hasScaleY = false, hasRotation = false;
};

void applyStoryboard(Pose& pose, const assets::UiLayout& layout, const std::string& storyboard, float time,
                     const std::string& target) {
    for (const auto& board : layout.storyboards) {
        if (board.name != storyboard) continue;
        for (const auto& track : board.tracks) {
            if (track.target != target) continue;
            const float value = evaluateTrack(track, time);
            switch (track.property) {
            case 1: pose.translateX = value; break;
            case 2: pose.translateY = value; break;
            case 3:
                pose.scaleX = value;
                pose.hasScaleX = true;
                break;
            case 4:
                pose.scaleY = value;
                pose.hasScaleY = true;
                break;
            case 5:
                pose.rotation = value;
                pose.hasRotation = true;
                break;
            case 6:
                pose.opacity = value;
                pose.hasOpacity = true;
                break;
            default: break;
            }
        }
    }
}

Pose poseOf(const assets::UiLayout* layout, const std::string& storyboard, float time, const std::string& target,
            const std::vector<PlayingStoryboard>& later) {
    Pose pose;
    if (!layout) return pose;
    if (!storyboard.empty()) applyStoryboard(pose, *layout, storyboard, time, target);
    for (const PlayingStoryboard& playing : later) applyStoryboard(pose, *layout, playing.name, playing.time, target);
    return pose;
}

struct Frame {
    float x, y, scaleX, scaleY, opacity;
};

class Drawer {
public:
    Drawer(Canvas& canvas, Resources& resources, const LayoutScene& scene) : canvas_(canvas), resources_(resources), scene_(scene) {}

    void layout(const std::string& path, Frame frame, const std::string& storyboard, const std::string& instance,
                float time, const std::vector<PlayingStoryboard>& later = {}) {
        const assets::UiLayout& parsed = resources_.layout(path);
        const assets::UiLayout* previous = current_;
        const std::string previousPath = currentPath_, previousBoard = storyboard_, previousInstance = instance_;
        const float previousTime = time_;
        const std::vector<PlayingStoryboard>* previousLater = later_;
        later_ = &later;
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
        later_ = previousLater;
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
        const Pose pose = poseOf(current_, storyboard_, time_, e.name, *later_);
        float width = e.size[0], height = e.size[1];
        const assets::Image* image = nullptr;
        if (e.type == assets::UiElementType::Image) {
            const std::string* replaced = find(scene_.images, e.name);
            image = &resources_.image(replaced ? *replaced : layoutTexturePath(currentPath_, e.source));
            if (width <= 1e-6f) width = float(image->width);
            if (height <= 1e-6f) height = float(image->height);
        }
        float scaleX = e.scale[0] * pose.scaleX, scaleY = e.scale[1] * pose.scaleY;
        float rotation = 0;
        if (scene_.rotations) {
            scaleX = pose.hasScaleX ? pose.scaleX : e.scale[0];
            scaleY = pose.hasScaleY ? pose.scaleY : e.scale[1];
            rotation = pose.hasRotation ? pose.rotation : e.rotation;
        }
        auto placed = instance_.empty() ? scene_.positions.end() : scene_.positions.find(instance_ + "/" + e.name);
        if (placed == scene_.positions.end()) placed = scene_.positions.find(e.name);
        const float baseX = placed != scene_.positions.end() ? placed->second[0] : e.position[0];
        const float baseY = placed != scene_.positions.end() ? placed->second[1] : e.position[1];
        const float localX = baseX + pose.translateX, localY = baseY + pose.translateY;
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
            if (rotation != 0)
                canvas_.drawRotated(*image, {0, 0, image->width, image->height}, frame.x + width * frame.scaleX / 2,
                                    frame.y + height * frame.scaleY / 2, width * frame.scaleX, height * frame.scaleY, rotation, tint);
            else
                canvas_.drawScaled(*image, {0, 0, image->width, image->height}, frame.x, frame.y, width * frame.scaleX,
                                   height * frame.scaleY, tint);
        } else if (e.type == assets::UiElementType::Reference || scene_.attachedLayouts.count(e.name)) {
            const auto attached = scene_.attachedLayouts.find(e.name);
            const auto board = scene_.referenceStoryboards.find(e.name);
            const auto referenceTime = scene_.referenceTimes.find(e.name);
            layout(attached != scene_.attachedLayouts.end() ? attached->second
                                                            : layoutFolder(currentPath_) + lowerCase(e.source) + ".bxaml",
                   frame, board == scene_.referenceStoryboards.end() ? std::string() : board->second, e.name,
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
        float anchor = frame.x + boxWidth / 2 + float(slot.offsetX);
        if (slot.align == Align::Left) anchor = frame.x + float(slot.offsetX);
        else if (slot.align == Align::Right) anchor = frame.x + boxWidth + float(slot.offsetX);
        const float blockHeight = float(pitch) * float(lines.size());
        float top = frame.y + (boxHeight - blockHeight) / 2 + float(slot.offsetY);
        Colour colour{e.colour[0] / 255.0f, e.colour[1] / 255.0f, e.colour[2] / 255.0f, e.colour[3] / 255.0f * frame.opacity};
        if (slot.tinted) colour = Colour{slot.tint[0], slot.tint[1], slot.tint[2], colour.a};
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
    const std::vector<PlayingStoryboard>* later_ = nullptr;
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

std::optional<std::array<float, 4>> layoutChildBox(const assets::UiLayout& layout, const std::vector<PlayingStoryboard>& storyboards,
                                                   const std::string& name) {
    for (const assets::UiElement& e : layout.root.children) {
        if (e.name != name) continue;
        const Pose pose = poseOf(&layout, std::string(), 0, name, storyboards);
        const float width = e.size[0], height = e.size[1];
        const float scaleX = e.scale[0] * pose.scaleX, scaleY = e.scale[1] * pose.scaleY;
        const float x = layout.root.position[0] + e.position[0] + pose.translateX + e.origin[0] * width * (1 - scaleX);
        const float y = layout.root.position[1] + e.position[1] + pose.translateY + e.origin[1] * height * (1 - scaleY);
        return std::array<float, 4>{x, y, width, height};
    }
    return std::nullopt;
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
                float y, float scale) {
    Drawer drawer(canvas, resources, scene);
    drawer.layout(layoutPath, {x, y, scale, scale, scene.opacity}, scene.storyboard, std::string(), scene.time,
                  scene.laterStoryboards);
}

}
