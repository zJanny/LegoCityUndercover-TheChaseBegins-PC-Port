#pragma once
// The level object class `particles`, kind 1 (docs/re/particles-objects.md): create 0x1a7f30, post-link 0x1990c4, update 0x1a8394, message handler
// 0x1b59f0. The record at entity +0x7c holds the definition (+0), ReTriggerTime (+0x10), TimeRandomiser (+0x14), the timer (+0xc), the emitter
// (+0x20) and the flag byte +0x2c. Ported: the trigger messages of an object with ReTriggerTime 0 or below and the timer that disables a
// one-shot object again. STAND-IN SCRIPT-S325: the self-starting branches of the update (ReTriggerTime above 0, -5, endless definitions), the
// damage, the sound, the screen shake, the path, the room test and the objects nested in class files are not ported. No SDL dependency.
#include "assets/assets.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr std::uint8_t kParticleObjectKind = 1;
constexpr std::uint8_t kParticleObjectMessageTrigger = 0xff;
constexpr std::uint8_t kParticleObjectMessageUntrigger = 0xfe;
constexpr float kParticleObjectEndlessDuration = 98.0f;   // 0x1a7f30: a definition duration above 98 sets flag 4

struct ParticleObjectParams {
    std::int32_t definition = 0;      // ParticleDef (0x2451d0)
    float reTriggerTime = 0;          // ReTriggerTime (0x241280)
    float timeRandomiser = 0;         // TimeRandomiser
    bool instance = false;            // Instance (+0xa): shares the emitter of an equal object instead of starting its own
    bool justMoveSpawnPoint = false;  // JustMoveSpawnPoint (+0x2c bit 0): the emitter has no parent node and starts at the node's position
    std::uint16_t sound = 0;          // ATTR_SFX (+4 holds a sound record when not 0)
    float damage = 0;                 // Damage (+8)
};

// What the class reads of a definition: 0x215600 gives +0x38 (-1 for an index out of range); the untrigger fades the emitter out over +0x80
// (0x18a0a4 is the emitter's definition, emitter +0x144).
struct ParticleObjectDefinition {
    float duration = -1;
    float lifetimeMax = 0;
};

struct ParticleObject {
    std::string instance;             // lower case
    std::array<float, 3> position{};  // initialposition: the node's world position (no parent attachment in the port)
    ParticleObjectParams params;
    ParticleObjectDefinition definition;
    bool endless = false;             // flag 4
    float timer = 0;                  // +0xc
    std::uint32_t emitter = 0;        // +0x20, 0 when none runs
    bool enabled = true;              // entity +0x10 bit 0 clear
};

enum class ParticleObjectEventKind { Started, Refused, Stopped, Disabled };
struct ParticleObjectEvent {
    ParticleObjectEventKind kind = ParticleObjectEventKind::Started;
    std::string instance;
    std::int32_t definition = 0;
    std::uint32_t emitter = 0;
    float fadeSeconds = 0;
};

class ParticleObjectWorld {
public:
    virtual ~ParticleObjectWorld() = default;
    // 0x23de24(0, definition, position, parent, 0, 0, 0): the emitter handle, 0 when refused.
    virtual std::uint32_t start(std::int32_t definition, const std::array<float, 3>& position) = 0;
    // 0x23ddb4: ends the emitter, with a fade when `fadeSeconds` is above 0.
    virtual void stop(std::uint32_t emitter, float fadeSeconds) = 0;
    // 0x2345dc: a random float in [0, 1).
    virtual float random() = 0;
    // STAND-IN SCRIPT-S325: the callback 0x2c7984 that the emitter calls is not read; the port takes an emitter that has ended as gone (+0x20 = 0).
    virtual bool running(std::uint32_t emitter) const = 0;
};

// The parameters 0x1a7f30 reads, with the class defaults of `particles.blvl` as fallbacks.
ParticleObjectParams particleObjectParams(const assets::LevelObject& object, const assets::Level& cls);

class ParticleObjects {
public:
    // Every level object of kind 1. The timer starts at the definition's duration, or at random x TimeRandomiser when that is above 0.
    void load(const assets::Level& level, const assets::ClassLookup& classOf, const std::function<ParticleObjectDefinition(std::int32_t)>& definitionOf,
              ParticleObjectWorld& world);
    // 0x1b59f0 with the messages TriggerObject sends: 0xff enables the object and starts the emitter at the node with a fresh timer, 0xfe ends the
    // emitter over the definition's +0x80 and disables the object. An object with ReTriggerTime above 0 ignores both.
    void message(const std::string& lowerInstance, std::uint8_t message, ParticleObjectWorld& world);
    // 0x1a8394 for the one-shot objects (flag 4 clear, ReTriggerTime 0): the timer runs down by `dt` (0x23602c) and below 0 the object is disabled
    // (0x2428c0) in every update; the emitter keeps running to its own end.
    void update(float dt, ParticleObjectWorld& world);

    const std::vector<ParticleObject>& objects() const { return objects_; }
    const ParticleObject* find(const std::string& lowerInstance) const;
    std::vector<ParticleObjectEvent> takeEvents();

private:
    float startTimer(const ParticleObject& object, ParticleObjectWorld& world) const;

    std::vector<ParticleObject> objects_;
    std::vector<ParticleObjectEvent> events_;
};

}
