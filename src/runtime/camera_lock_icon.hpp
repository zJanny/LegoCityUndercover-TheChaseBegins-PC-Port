#pragma once
// The crossed-out camera icon (Sprites/UI_HUD/UI_CameraLocked.tga) of the HUD: object *0x3758b0, built at 0x193a1c with a
// fade of 0.3 s, triggered by 0x193aa4 through 0x23cc00(0.5, object, 0) and drawn by 0x1b1150 at (10, 240 - 10 - height)
// of the top screen with alpha = fade * 255. The four states and their durations were read from the object in the
// oracle (state byte +0x10, timer +0x28, fade times +0x14 / +0x18, hold +0x1c; docs/re/camera.md). No SDL dependency.
//
// The conditions 0x21d744() == 0 and 0x236008() == 0 that 0x193aa4 also tests are not replicated (read as 0).
namespace chase::runtime {

class CameraLockIcon {
public:
    static constexpr float kFadeSeconds = 0.3f;  // +0x14 and +0x18, literal 0x3e99999a at 0x193a74
    static constexpr float kHoldSeconds = 0.5f;  // +0x1c, argument of 0x23cc00 in 0x193aa4

    enum class State { Hidden, FadingIn, Shown, FadingOut };

    // 0x23cc00: called by 0x193aa4 while L or R is pressed and the camera command is not the follow controller.
    void trigger();
    void update(float dt);
    float alpha() const;
    State state() const { return state_; }

private:
    State state_ = State::Hidden;
    float now_ = 0, start_ = 0;
};

}
