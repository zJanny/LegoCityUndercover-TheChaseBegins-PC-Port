#include "qol/pc_mouse_camera.hpp"

#include <algorithm>

namespace qol {

bool PixelRect::contains(PixelPoint point) const {
    return width > 0 && height > 0 && point.x >= x && point.x < x + width && point.y >= y && point.y < y + height;
}

void MouseCamera::update(bool wanted, bool buttonHeld, PixelPoint realPointer, const MouseScreen& screen) {
    if (!wanted) {
        captured_ = false;
        pixels_ = 0;
        release_.reset();
        return;
    }
    if (captured_ || buttonHeld || screen.lower.contains(realPointer)) return;
    captured_ = true;
    pointer_ = realPointer;
    pixels_ = 0;
}

void MouseCamera::motion(int dx, int dy, const MouseScreen& screen) {
    if (!captured_) return;
    pixels_ += dx;
    pointer_.x = std::clamp(pointer_.x + dx, 0, std::max(screen.width - 1, 0));
    pointer_.y = std::clamp(pointer_.y + dy, 0, std::max(screen.height - 1, 0));
    if (!screen.lower.contains(pointer_)) return;
    captured_ = false;
    release_ = pointer_;
}

std::optional<PixelPoint> MouseCamera::takeRelease() {
    std::optional<PixelPoint> point = release_;
    release_.reset();
    return point;
}

float MouseCamera::take(float dt) {
    const long pixels = pixels_;
    pixels_ = 0;
    return dt > 0.0f ? static_cast<float>(pixels) * mouseShoulderSecondsPerPixel / dt : 0.0f;
}

}
