#pragma once
// Use markers (docs/re/use-spot.md): the spinning disc drawn on the ground at a usable zone. The textures load at 0x162adc
// into the table 0x5295ec; the update 0x172254 shows a marker for each zone of the list 0x5289ec (built by 0x1c015c) that is
// usable and within kUseMarkerRange of the camera or the player; a marker is one of 32 fader slots (0x529618, 0x30 bytes:
// init 0x23cb90, show 0x23cc00, value 0x235e2c) and the slot callback 0x17218c draws it with 0x1b29f0. No SDL dependency.
#include <array>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr unsigned kUseMarkerSlots = 32;
constexpr float kUseMarkerFadeInSeconds = 0.15f;   // 0x23cb90 call at 0x162c68
constexpr float kUseMarkerFadeOutSeconds = 0.15f;
constexpr float kUseMarkerHoldSeconds = 0.1f;      // 0x23cc00 call at 0x1725f0
constexpr float kUseMarkerRange = 25.0f;           // 0x1c015c writes it to the zone record +8
constexpr float kUseMarkerHalfSize = 1.0f;         // the quad of 0x1b29f0
constexpr float kUseMarkerLift = 0.1f;
constexpr float kUseMarkerSpinDivisor = 240.0f;    // spin angle in radians = clock / 240 (0x1b2a44)
constexpr int kUseMarkerReleaseAlpha = 4;          // 0x17218c frees the slot below this alpha (x 255) while fading out
// STAND-IN SCRIPT-S247: the clock 0x241098 advanced by 2 per drawn frame at 30 updates per second in the oracle; its
// multiplier (*(mode + 0xc) + 0xc) was not read.
constexpr float kUseMarkerClockUnitsPerSecondStandIn = 60.0f;

// Index into the table 0x5295ec (the order of the loads at 0x162b54..0x162c24); 0x292388 picks it from the entity kind and
// gives 0 (useGeneric) to the kinds it does not list, such as props/prop_simpleuse (165).
constexpr const char* kUseMarkerTextureDirectory = "sprites/game/usemarkers/";
constexpr std::array<const char*, 11> kUseMarkerTextures = {"usegeneric", "usepolice", "userobber", "useminer", "useastronaut", "usefarmer",
                                                            "usechicken", "usepig", "usefireman", "useconstruction", "uselocked"};

struct UseMarkerCandidate {
    std::string instance;
    std::array<float, 16> matrix{};  // entity node rows right, up, forward, translation (assets::placementMatrix)
    std::array<float, 3> ground{};   // marker position in the world (entry +8 of 0x1db800, in world space)
    bool usable = false;             // the tests of 0x172254 before the distance test
    int texture = 0;
};

struct UseMarkerVertex {
    float position[3];
    float uv[2];
};

struct UseMarkerQuad {
    int texture = 0;
    float alpha = 0;
    std::array<UseMarkerVertex, 4> vertices{};
};

class UseMarkers {
public:
    void update(const std::vector<UseMarkerCandidate>& candidates, const float camera[3], const float player[3], float dt);
    std::vector<UseMarkerQuad> quads() const;
    // 0x235e2c of the marker of `instance`, 0 without one.
    float alpha(const std::string& instance) const;
    // A slot holds the marker of `instance` (from the first show until 0x17218c frees it).
    bool slotted(const std::string& instance) const;
    float clock() const { return clock_; }

private:
    enum class FadeState { Hidden, FadingIn, Shown, FadingOut };
    struct Slot {
        bool used = false;
        UseMarkerCandidate marker;
        FadeState state = FadeState::Hidden;
        float elapsed = 0, hold = 0;
        float value() const;
    };
    static void show(Slot& slot);
    static void step(Slot& slot, float dt);

    std::array<Slot, kUseMarkerSlots> slots_{};
    float clock_ = 0;
};

}
