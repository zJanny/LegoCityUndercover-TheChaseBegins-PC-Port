#include "runtime/particle_object.hpp"

#include "assets/object_classes.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace chase::runtime {
namespace {

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

const assets::LevelParam* param(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    return effectiveParam(object, cls, assets::nameHash(name));
}

float floatParam(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    const assets::LevelParam* found = param(object, cls, name);
    if (!found || found->type != 0) return 0.0f;
    float value;
    std::memcpy(&value, &found->value, 4);
    return value;
}

std::int32_t intParam(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    const assets::LevelParam* found = param(object, cls, name);
    if (!found) return 0;
    if (found->type == 0) return std::int32_t(floatParam(object, cls, name));
    return std::int32_t(found->value);
}

std::array<float, 3> positionOf(const assets::LevelObject& object, const assets::Level& cls) {
    std::array<float, 3> position{};
    const assets::LevelParam* found = param(object, cls, "initialposition");
    if (found && found->type == 2 && !found->blocks.empty() && found->blocks[0].size() >= 12) std::memcpy(position.data(), found->blocks[0].data(), 12);
    return position;
}

}

ParticleObjectParams particleObjectParams(const assets::LevelObject& object, const assets::Level& cls) {
    ParticleObjectParams params;
    params.definition = intParam(object, cls, "ParticleDef");
    params.reTriggerTime = floatParam(object, cls, "ReTriggerTime");
    params.timeRandomiser = floatParam(object, cls, "TimeRandomiser");
    params.instance = intParam(object, cls, "Instance") != 0;
    params.justMoveSpawnPoint = intParam(object, cls, "JustMoveSpawnPoint") != 0;
    params.sound = std::uint16_t(intParam(object, cls, "ATTR_SFX"));
    params.damage = floatParam(object, cls, "Damage");
    return params;
}

float ParticleObjects::startTimer(const ParticleObject& object, ParticleObjectWorld& world) const {
    if (object.params.timeRandomiser <= 0.0f) return object.definition.duration;
    return world.random() * object.params.timeRandomiser;
}

void ParticleObjects::load(const assets::Level& level, const assets::ClassLookup& classOf,
                           const std::function<ParticleObjectDefinition(std::int32_t)>& definitionOf, ParticleObjectWorld& world) {
    objects_.clear();
    events_.clear();
    for (const assets::LevelObject& placed : level.objects) {
        const assets::Level* cls = classOf(placed.className);
        if (!cls || assets::objectKind(placed, *cls) != kParticleObjectKind) continue;
        ParticleObject object;
        object.instance = lower(placed.instance);
        object.position = positionOf(placed, *cls);
        object.params = particleObjectParams(placed, *cls);
        object.definition = definitionOf(object.params.definition);
        object.endless = object.definition.duration > kParticleObjectEndlessDuration;
        object.timer = startTimer(object, world);
        objects_.push_back(std::move(object));
    }
}

void ParticleObjects::message(const std::string& lowerInstance, std::uint8_t message, ParticleObjectWorld& world) {
    if (message != kParticleObjectMessageTrigger && message != kParticleObjectMessageUntrigger) return;
    for (ParticleObject& object : objects_) {
        if (object.instance != lowerInstance || object.params.reTriggerTime > 0.0f) continue;
        if (object.emitter && !world.running(object.emitter)) object.emitter = 0;
        if (message == kParticleObjectMessageTrigger) {
            object.enabled = true;
            if (object.emitter) continue;
            object.emitter = world.start(object.params.definition, object.position);
            if (!object.emitter) {
                events_.push_back({ParticleObjectEventKind::Refused, object.instance, object.params.definition, 0, 0});
                continue;
            }
            object.endless = object.definition.duration > kParticleObjectEndlessDuration;
            object.timer = startTimer(object, world);
            events_.push_back({ParticleObjectEventKind::Started, object.instance, object.params.definition, object.emitter, 0});
            continue;
        }
        if (object.emitter) {
            world.stop(object.emitter, object.definition.lifetimeMax);
            events_.push_back({ParticleObjectEventKind::Stopped, object.instance, object.params.definition, object.emitter, object.definition.lifetimeMax});
            object.emitter = 0;
        }
        if (object.enabled) events_.push_back({ParticleObjectEventKind::Disabled, object.instance, object.params.definition, 0, 0});
        object.enabled = false;
    }
}

void ParticleObjects::update(float dt, ParticleObjectWorld& world) {
    for (ParticleObject& object : objects_) {
        if (object.endless || object.params.reTriggerTime != 0.0f) continue;
        if (object.emitter && !world.running(object.emitter)) object.emitter = 0;
        if (object.timer < 0.0f && object.enabled) {
            object.enabled = false;
            events_.push_back({ParticleObjectEventKind::Disabled, object.instance, object.params.definition, 0, 0});
        }
        object.timer -= dt;
    }
}

const ParticleObject* ParticleObjects::find(const std::string& lowerInstance) const {
    for (const ParticleObject& object : objects_)
        if (object.instance == lowerInstance) return &object;
    return nullptr;
}

std::vector<ParticleObjectEvent> ParticleObjects::takeEvents() {
    std::vector<ParticleObjectEvent> taken;
    taken.swap(events_);
    return taken;
}

}
