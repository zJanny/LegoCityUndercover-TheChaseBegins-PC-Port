#pragma once
// Script natives that change how an object is drawn (docs/re/script-natives-world.md): EffectFadeObject 0x288a60 (node tween registry
// 0x21b7cc / 0x1593c8), EnableDistanceFade 0x2a05bc and AddToSemiTransNearCameraList 0x2cb734. The draw paths that read the node alpha, the
// distance fade byte and the semi-transparent list are not ported, so the state is recorded here and the host applies the tween value to the
// object's instances. No SDL dependency.
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

enum class EffectNativeKind {
    EffectFadeObject,
    EnableDistanceFade,
    AddToSemiTransNearCameraList,
};

std::optional<EffectNativeKind> effectNativeKind(const std::string& lowerCaseName);
std::vector<std::string> effectNativeNames();
unsigned effectNativeArguments(EffectNativeKind kind);

constexpr std::uint8_t kFadeFlagAlpha = 1;
constexpr std::uint8_t kFadeFlagColour = 2;
constexpr std::uint8_t kFadeFlagThirdProperty = 4;
// 0x21b840: a full table (20 entries of 28 bytes at manager +0x338) completes its oldest entry before a new node is added.
constexpr std::size_t kNodeTweenLimit = 20;

// What the tween leaves on an object's node: `multiplier` is int(value x 255) / 255 when the flags carry the alpha or the colour bit (1.0 otherwise),
// `hidden` is the node's hidden state (0x23f734: hidden at value 0, shown otherwise, hidden when a finished tween ends at 0).
struct ObjectFade {
    float multiplier = 1.0f;
    bool hidden = false;
};

struct SemiTransNearCameraEntry {
    std::string model;
    std::string tag;
    bool operator==(const SemiTransNearCameraEntry& other) const { return model == other.model && tag == other.tag; }
};

class EffectNativeState {
public:
    // 0x21b7cc(manager, node, flags, from, to, duration, 0): the object's node starts a tween; the `from` value is applied at once.
    void fadeObject(const std::string& lowerCaseInstance, float from, float to, float duration, std::uint8_t flags);
    // 0x1593c8 with the time since registration, for every entry once per update.
    void update(float dt);
    std::optional<ObjectFade> fade(const std::string& lowerCaseInstance) const;
    std::size_t activeFades() const { return entries_.size(); }
    bool anyFade() const { return !faded_.empty(); }

    // 0x2a05bc: the distance fade bit (bit 4 of byte +0xde of the object's mesh nodes) as the last script wrote it; unset until then.
    void enableDistanceFade(const std::string& lowerCaseInstance, float value) { distanceFade_[lowerCaseInstance] = value != 0.0f; }
    std::optional<bool> distanceFade(const std::string& lowerCaseInstance) const;

    // 0x2cb734: the model name (lower case, without the quotes) and the entry name of the call; one entry per pair.
    void addSemiTransNearCamera(const std::string& lowerCaseModel, const std::string& tag);
    const std::vector<SemiTransNearCameraEntry>& semiTransNearCamera() const { return semiTransNearCamera_; }

private:
    struct Entry {
        std::string instance;
        float from = 0, to = 0, duration = 0, elapsed = 0, value = 0;
        std::uint8_t flags = 0;
    };

    void apply(const Entry& entry, float value);
    void evaluate(std::size_t index, float elapsed);
    bool finishes(std::size_t index);

    std::vector<Entry> entries_;
    std::map<std::string, ObjectFade> faded_;
    std::map<std::string, bool> distanceFade_;
    std::vector<SemiTransNearCameraEntry> semiTransNearCamera_;
};

}
