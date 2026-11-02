// Mouse camera of the PC controls feature (QOLCTL-6): mouse X feeds the existing camera turn, the cursor is captured in play, released
// in menus and while the hidden pointer is on the lower screen; a left click on the lower screen stays a touch.
// Spec: docs/design/qol-controls.md entry 2026-11-01T13:50:47 (Q1, Q4) with the answers of 2026-11-02 ("Stand-in + setting" for the
// sensitivity QOLCTL-S5, "Virtual pointer" for the lower screen). Cases written by 20261102-qolctl2-B on 2026-11-02; no value is
// recorded from the original: the sensitivity is a stand-in and the turn goes through the recorded shoulderTurnInput (R value 1 gives 2,
// tests/runtime_test.cpp) and the recorded turn gain 0.6 of the OnFoot preset (follow_camera.hpp).
#include "qol/active_settings.hpp"
#include "qol/mouse_sensitivity.hpp"
#include "qol/pc_mouse_camera.hpp"
#include "qol/pc_mouse_camera_sdl.hpp"
#include "runtime/follow_camera.hpp"

#include <SDL.h>
#include <cmath>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

bool near(float a, float b, float tolerance = 1e-5f) { return std::fabs(a - b) <= tolerance; }

constexpr float kDt = 1.0f / 30.0f;

const qol::MouseScreen screen{800, 600, {100, 400, 600, 200}};
const qol::MouseScreen hiddenLower{800, 600, {0, 0, 0, 0}};

void setFeature(bool on) {
    qol::Settings settings;
    if (on) settings.setMaster(qol::Master::Improved);
    qol::setActiveSettings(settings);
}

void originalModeFirst() {
    qol::MouseCamera camera;
    camera.update(false, false, {400, 100}, screen);
    camera.motion(50, 0, screen);
    require(!camera.captured(), "feature off: the cursor is never captured");
    require(camera.take(kDt) == 0.0f, "feature off: no motion gives no turn");

    setFeature(false);
    qol::MouseCameraSdl adapter;
    SDL_Event motion{};
    motion.type = SDL_MOUSEMOTION;
    motion.motion.xrel = 30;
    adapter.event(motion, nullptr, screen);
    require(adapter.take(kDt) == 0.0f, "feature off through SDL: the motion event gives no turn");
}

void motionMakesTheTurn() {
    qol::MouseCamera camera;
    camera.update(true, false, {400, 100}, screen);
    require(camera.captured(), "feature on, pointer in the top screen: captured");
    require(camera.virtualPointer().x == 400 && camera.virtualPointer().y == 100, "the hidden pointer starts at the real one");
    camera.motion(10, 0, screen);
    camera.motion(8, 3, screen);
    const float value = camera.take(kDt);
    require(near(value, 18 * qol::mouseSecondsPerPixel() / kDt), "pixels of one update add up to one analog value");
    require(camera.take(kDt) == 0.0f, "taking clears the accumulation");
    camera.motion(-5, 0, screen);
    require(camera.take(kDt) < 0.0f, "motion to the left is negative");
    camera.motion(5, 0, screen);
    require(camera.take(0.0f) == 0.0f, "no turn without time");
}

void turnGoesThroughTheExistingCameraTurn() {
    using namespace chase::runtime;
    auto yawAfter = [](float shoulder) {
        FollowCamera camera;
        const float target[3] = {0, 0, 0};
        camera.reset(target, 0.0f, 0.0f);
        const float before = camera.yaw();
        FollowCameraInput input;
        input.dt = kDt;
        input.shoulderTurn = shoulder;
        camera.update(input);
        return camera.yaw() - before;
    };
    qol::MouseCamera mouse;
    mouse.update(true, false, {400, 100}, screen);
    mouse.motion(18, 0, screen);
    const float rightValue = mouse.take(kDt);
    const float buttonYaw = yawAfter(shoulderTurnInput(0, 1, false));
    const float mouseYaw = yawAfter(shoulderTurnInput(0, rightValue, false));
    require(buttonYaw > 0.0f, "R turns the camera towards +yaw");
    require(mouseYaw > 0.0f && near(mouseYaw / buttonYaw, rightValue, 0.01f), "mouse to the right turns like R, in proportion to its analog size");
    mouse.motion(-18, 0, screen);
    const float leftYaw = yawAfter(shoulderTurnInput(0, mouse.take(kDt), false));
    require(near(leftYaw, -mouseYaw, 1e-4f), "mouse to the left turns the other way by the same amount");
    mouse.motion(18, 0, screen);
    const float invertedYaw = yawAfter(shoulderTurnInput(0, mouse.take(kDt), true));
    require(near(invertedYaw, -mouseYaw, 1e-4f), "the Invert camera option flips the mouse turn like the shoulder buttons");
    mouse.motion(0, 40, screen);
    require(mouse.take(kDt) == 0.0f, "mouse Y gives no turn (no pitch)");
}

void menusReleaseTheCursor() {
    qol::MouseCamera camera;
    camera.update(true, false, {400, 100}, screen);
    camera.motion(12, 0, screen);
    camera.update(false, false, {400, 100}, screen);
    require(!camera.captured(), "a menu (settings menu, pause screen) releases the cursor");
    require(camera.take(kDt) == 0.0f, "the motion before the menu is dropped");
    camera.motion(12, 0, screen);
    require(camera.take(kDt) == 0.0f, "no motion counts while released");
    camera.update(true, false, {400, 100}, screen);
    require(camera.captured(), "the cursor is captured again when the menu closes");
}

void lowerScreenUsesTheHiddenPointer() {
    qol::MouseCamera camera;
    camera.update(true, false, {400, 100}, screen);
    camera.motion(0, 250, screen);
    require(camera.captured() && !camera.takeRelease(), "the hidden pointer above the lower screen keeps the capture");
    camera.motion(20, 60, screen);
    require(!camera.captured(), "moving the hidden pointer into the lower screen releases the capture");
    const std::optional<qol::PixelPoint> release = camera.takeRelease();
    require(release && release->x == 420 && release->y == 410, "the real cursor is put where the hidden pointer is");
    require(!camera.takeRelease(), "the release is reported once");
    camera.update(true, false, {420, 410}, screen);
    require(!camera.captured(), "the cursor stays free while it is on the lower screen");
    camera.update(true, false, {420, 399}, screen);
    require(camera.captured(), "moving the cursor back out of the lower screen captures again");

    qol::MouseCamera edge;
    edge.update(true, false, {790, 590}, hiddenLower);
    edge.motion(500, 500, hiddenLower);
    require(edge.captured() && edge.virtualPointer().x == 799 && edge.virtualPointer().y == 599, "the hidden pointer stays inside the window");
    edge.motion(0, 0, hiddenLower);
    require(edge.captured(), "a hidden lower screen (Improved) never releases the cursor");

    qol::MouseCamera start;
    start.update(true, false, {300, 500}, screen);
    require(!start.captured(), "a pointer that starts on the lower screen is not captured");
}

void aHeldButtonKeepsTheTouch() {
    qol::MouseCamera camera;
    camera.update(true, true, {400, 100}, screen);
    require(!camera.captured(), "a held mouse button (a touch drag) does not start a capture");
    camera.update(true, false, {400, 100}, screen);
    require(camera.captured(), "released button: captured");
}

void sdlAdapter() {
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SKIP: no dummy video driver: " << SDL_GetError() << '\n';
        return;
    }
    SDL_Window* window = SDL_CreateWindow("mouse camera", 0, 0, 800, 600, 0);
    require(window != nullptr, "window for the dummy driver");
    if (!window) return;
    SDL_RaiseWindow(window);
    SDL_PumpEvents();
    SDL_SetWindowInputFocus(window);
    SDL_PumpEvents();
    if (!(SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS)) {
        std::cerr << "SKIP: the dummy window has no input focus\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }
    SDL_WarpMouseInWindow(window, 400, 100);
    SDL_PumpEvents();
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

    setFeature(false);
    {
        qol::MouseCameraSdl adapter;
        adapter.frame(window, false, screen);
        require(SDL_GetRelativeMouseMode() == SDL_FALSE, "original mode: the cursor is not captured");
    }
    setFeature(true);
    {
        qol::MouseCameraSdl adapter;
        adapter.frame(window, true, screen);
        require(SDL_GetRelativeMouseMode() == SDL_FALSE, "feature on, menu open: not captured");
        adapter.frame(window, false, screen);
        require(SDL_GetRelativeMouseMode() == SDL_TRUE, "feature on, in play: captured");
        SDL_Event motion{};
        motion.type = SDL_MOUSEMOTION;
        motion.motion.xrel = 18;
        motion.motion.yrel = 0;
        adapter.event(motion, window, screen);
        require(adapter.take(kDt) > 0.0f, "a motion event while captured turns");
        motion.motion.xrel = 0;
        motion.motion.yrel = 250;
        adapter.event(motion, window, screen);
        require(SDL_GetRelativeMouseMode() == SDL_TRUE, "above the lower screen: still captured");
        motion.motion.yrel = 60;
        adapter.event(motion, window, screen);
        require(SDL_GetRelativeMouseMode() == SDL_FALSE, "the hidden pointer reached the lower screen: released");
        adapter.frame(window, true, screen);
        SDL_Event lost{};
        lost.type = SDL_WINDOWEVENT;
        lost.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
        adapter.event(lost, window, screen);
        require(SDL_GetRelativeMouseMode() == SDL_FALSE, "losing the focus releases the cursor");
    }
    require(SDL_GetRelativeMouseMode() == SDL_FALSE, "the adapter releases the cursor when it ends");
    SDL_DestroyWindow(window);
    SDL_Quit();
}

}

int main() {
    originalModeFirst();
    motionMakesTheTurn();
    turnGoesThroughTheExistingCameraTurn();
    menusReleaseTheCursor();
    lowerScreenUsesTheHiddenPointer();
    aHeldButtonKeepsTheTouch();
    sdlAdapter();
    if (failures) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "qol-pc-mouse-camera: ok\n";
    return 0;
}
