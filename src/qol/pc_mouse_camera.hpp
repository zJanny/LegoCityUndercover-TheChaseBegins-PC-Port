#pragma once

#include <optional>

namespace qol {

struct PixelPoint {
    int x = 0, y = 0;
};

struct PixelRect {
    int x = 0, y = 0, width = 0, height = 0;
    bool contains(PixelPoint point) const;
};

struct MouseScreen {
    int width = 0, height = 0;
    PixelRect lower;
};

inline constexpr float mouseShoulderSecondsPerPixel = 0.0018f;

class MouseCamera {
public:
    bool captured() const { return captured_; }
    PixelPoint virtualPointer() const { return pointer_; }

    void update(bool wanted, bool buttonHeld, PixelPoint realPointer, const MouseScreen& screen);
    void motion(int dx, int dy, const MouseScreen& screen);
    std::optional<PixelPoint> takeRelease();
    float take(float dt);

private:
    bool captured_ = false;
    PixelPoint pointer_;
    long pixels_ = 0;
    std::optional<PixelPoint> release_;
};

}
