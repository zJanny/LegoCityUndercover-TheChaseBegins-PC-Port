#pragma once
// Build-its (object type 0x15, class props/prop_buildit): the state machine of the class record at entity+0x7c. Update 0x16cc78 runs the
// check 0x1d428c while the state equals the target (0x2424d0) and then the state change 0x1aad90; the message 3 of the use zone is
// handled in 0x176160. Rules and measurements: docs/re/buildit.md. No SDL dependency.
#include <cstddef>
#include <cstdint>
#include <vector>

namespace chase::runtime {

enum class BuilditPhase : std::uint16_t { Ready = 2, Building = 8, BitDone = 9, Finishing = 10, Finished = 11 };

constexpr float kBuilditStartRate = 0.1f;
constexpr float kBuilditRateStep = 0.006f;
constexpr float kBuilditRateMax = 0.5f;
constexpr float kBuilditSpinStep = 12.0f;
constexpr float kBuilditSpinStepLate = 24.0f;
constexpr float kBuilditSpinLateFrom = 90.0f;
constexpr float kBuilditSpinEnd = 180.0f;
constexpr float kBuilditDefaultStudValue = 25.0f;

struct BuilditState {
    std::uint16_t state = static_cast<std::uint16_t>(BuilditPhase::Ready);   // record +2
    std::uint16_t target = static_cast<std::uint16_t>(BuilditPhase::Ready);  // record +4
    int bit = -1;                                                            // record +0x35, the brick being built (-1 before the first)
    float rate = kBuilditStartRate;                                          // record +0x40
    float spin = 0;                                                          // record +0x48, degrees of the final prop's rise
    std::vector<float> progress;                                             // per brick +0x24
    bool built = false;                                                      // record +0x37 bit 4
    std::vector<float> flight;  // per brick: the progress the brick's node was last placed at (0x19c9e0 with the progress before its step; 1 after the change to 9; 0 after the change to 11)
    float propRise = 0;         // height of the final prop above its base: sin of the spin before its step, set by the check of state 10 (0x1d4320), 0 after the change to 11
    explicit BuilditState(int bitCount = 0) : progress(static_cast<std::size_t>(bitCount), 0.0f), flight(static_cast<std::size_t>(bitCount), 0.0f) {}
};

enum class BuilditEvent {
    BrickStarted,    // the change to state 8 picked the next brick
    BrickDone,       // the change to state 9 (sound ATTR_SFXBitDone, record +0x88)
    FinalPropShown,  // the change to state 10: the parts hide, the final prop appears
    Built,           // the change to state 11 (sound ATTR_SFXBuilt, record +0x8a): final prop enabled, parts disabled, studs
};

// One update of the object: the check of a state that reached its target, then the state change (0x16cc78).
std::vector<BuilditEvent> builditUpdate(BuilditState& build);

// Message 3 of the use zone (0x176160). `buttonHeld` is the action button; only the states 2, 3 and 4 take it (target 8).
// Returns the handler result: 1 usable, 2 not usable (state 10 and up).
int builditUse(BuilditState& build, bool buttonHeld);

// The states 2, 3 and 4 take the action button (target 8).
bool builditTakesAction(const BuilditState& build);

// Height of the final prop above its placement while it rises (state 10): sin of the spin angle in degrees, one metre of amplitude.
float builditPropRise(float spinDegrees);

}
