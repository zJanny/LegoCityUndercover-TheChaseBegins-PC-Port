#include "runtime/ai_spawner.hpp"

#include "assets/object_classes.hpp"
#include "runtime/level_start.hpp"
#include "runtime/switch_base.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {

constexpr std::uint8_t kCreateFlagNeverCreated = 1;

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

float floatParam(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param || param->type != 0) return 0.0f;
    float value;
    std::memcpy(&value, &param->value, 4);
    return value;
}

std::int32_t framesOf(float seconds) {
    const float frames = seconds * float(kAiSpawnerUpdatesPerSecond);
    if (std::isnan(frames)) return 0;
    if (frames >= 2147483647.0f) return 2147483647;
    if (frames <= -2147483648.0f) return -2147483647 - 1;
    return std::int32_t(frames);
}

std::string stringParam(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param || param->type != 1 || param->blocks.empty()) return {};
    const assets::Bytes& block = param->blocks[0];
    const auto end = std::find(block.begin(), block.end(), std::uint8_t(0));
    return std::string(block.begin(), end);
}

std::string referenceParam(const assets::LevelObject& object, const assets::Level& cls, const char* name,
                           const std::map<std::uint32_t, std::string>& names) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param) return {};
    const auto found = names.find(assets::objectReferenceHash(*param));
    return found == names.end() ? std::string() : found->second;
}

std::uint16_t bit(std::size_t slot) { return std::uint16_t(1u << slot); }

AiSpawnerEvent eventOf(AiSpawnerEventKind kind, const std::string& spawner) {
    AiSpawnerEvent event;
    event.kind = kind;
    event.spawner = spawner;
    return event;
}

}

const char* aiSpawnerStateName(AiSpawnerState state) {
    switch (state) {
    case AiSpawnerState::Idle: return "idle";
    case AiSpawnerState::Active: return "active";
    case AiSpawnerState::Spawn: return "spawn";
    case AiSpawnerState::PlayerOutside: return "player outside";
    }
    return "?";
}

AiSpawner aiSpawnerOf(const assets::LevelObject& object, const assets::Level& cls, const std::map<std::uint32_t, std::string>& names,
                      const assets::Matrix& matrix) {
    AiSpawner spawner;
    spawner.instance = lower(object.instance);
    spawner.matrix = matrix;
    AiSpawnerParams& params = spawner.params;
    params.mesh = stringParam(object, cls, "MeshName");
    params.sfxPlayFrames = framesOf(floatParam(object, cls, "SFXPlayTime"));
    params.sfx = std::uint16_t(switchIntParam(object, cls, "ATTR_SFX", 0));
    params.waitForDead = (switchIntParam(object, cls, "WaitForDead", 0) & 1) != 0;
    params.spawnFromSpawner = (switchIntParam(object, cls, "SpawnFromSpawner", 1) & 1) != 0;
    params.spawnOnScreen = (switchIntParam(object, cls, "SpawnOnScreen", 1) & 1) != 0;
    params.revealObject = referenceParam(object, cls, "RevealObject", names);
    const std::uint32_t respawn = std::uint32_t(framesOf(floatParam(object, cls, "respawntime")));
    params.respawnFrames = std::uint8_t(respawn >= 255 ? 255 : respawn);
    params.staggerFrames = std::uint8_t(framesOf(floatParam(object, cls, "staggertime")) & 0xff);
    params.maxSpawn = std::int16_t(switchIntParam(object, cls, "MaxSpawn", 0));
    params.startActive = switchIntParam(object, cls, "StartActive", 1) != 0;
    params.startState = std::uint16_t(switchIntParam(object, cls, "StartState", 0));
    spawner.start = aiStartStateOf(params.startState, object, cls, names);
    params.startEffect = referenceParam(object, cls, "StartEffect", names);
    params.startSpawned = switchIntParam(object, cls, "StartSpawned", 1) != 0;
    params.maxOnScreen = std::uint16_t(switchIntParam(object, cls, "MaxOnScreen", 0));
    params.playerInsideBound = stringParam(object, cls, "PlayerInsideBound");
    spawner.maxSpawn = params.maxSpawn;
    spawner.requested = spawner.state = params.startActive ? AiSpawnerState::Active : AiSpawnerState::Idle;
    spawner.revealPending = !params.revealObject.empty();
    for (std::size_t i = 1; i <= kAiSpawnerTargetParams; ++i) {
        const std::string name = "target" + std::to_string(i);
        std::string character = referenceParam(object, cls, name.c_str(), names);
        if (!character.empty()) spawner.slots.push_back({std::move(character), 0, {}});
    }
    return spawner;
}

void AiSpawnerSet::load(const assets::Level& level, const assets::ClassLookup& classOf,
                        const std::function<std::optional<assets::Matrix>(const std::string&)>& characterMatrix) {
    spawners_.clear();
    events_.clear();
    std::map<std::uint32_t, std::string> names;
    for (const assets::LevelObject& object : level.objects) names[assets::nameHash(object.instance)] = lower(object.instance);
    for (const assets::LevelObject& object : level.objects) {
        if (object.createFlags & kCreateFlagNeverCreated) continue;
        const assets::Level* cls = classOf(object.className);
        if (!cls || assets::objectKind(object, *cls) != kAiSpawnerKind) continue;
        const std::optional<assets::Placement> placement = assets::objectStart(level, object.instance);
        AiSpawner spawner = aiSpawnerOf(object, *cls, names, placement ? assets::placementMatrix(*placement) : assets::Matrix{});
        if (!spawner.params.spawnFromSpawner)
            for (AiSpawnerSlot& slot : spawner.slots)
                if (const auto matrix = characterMatrix(slot.character)) slot.placed = *matrix;
        spawners_.push_back(std::move(spawner));
    }
}

const AiSpawner* AiSpawnerSet::find(const std::string& instance) const {
    const auto found = std::find_if(spawners_.begin(), spawners_.end(), [&](const AiSpawner& s) { return s.instance == instance; });
    return found == spawners_.end() ? nullptr : &*found;
}

void AiSpawnerSet::hold(AiSpawner& spawner, std::size_t slot, AiSpawnerWorld& world) {
    world.hold(spawner.slots[slot].character);
    spawner.slots[slot].countdown = 0;
    spawner.heldMask |= bit(slot);
    if (spawner.params.maxOnScreen != 0) --spawner.onScreen;
}

void AiSpawnerSet::postLink(AiSpawnerWorld& world) {
    for (AiSpawner& spawner : spawners_) {
        spawner.onScreen = std::int16_t(spawner.slots.size());
        for (std::size_t i = 0; i < spawner.slots.size(); ++i) {
            spawner.slots[i].countdown = 0;
            if (!spawner.params.startSpawned || (spawner.params.maxOnScreen != 0 && spawner.params.maxOnScreen <= i)) {
                hold(spawner, i, world);
            } else if (spawner.maxSpawn <= 0 || spawner.maxSpawn > std::int32_t(spawner.spawned)) {
                spawner.slots[i].countdown = spawner.params.respawnFrames;
                ++spawner.spawned;
            }
        }
        if (spawner.maxSpawn < 0) spawner.maxSpawn = std::int16_t(spawner.slots.size());
        if (spawner.revealPending) world.disable(spawner.params.revealObject);
    }
}

void AiSpawnerSet::reset(AiSpawner& spawner, AiSpawnerWorld& world) {
    if (!world.referenceIdle(spawner.instance)) world.deactivateReference(spawner.instance);
    spawner.requested = spawner.params.startActive ? AiSpawnerState::Active : AiSpawnerState::Idle;
    spawner.spawned = 0;
    spawner.stagger = 0;
    for (std::size_t i = 0; i < spawner.slots.size(); ++i) {
        const bool limitReached = spawner.maxSpawn > 0 && spawner.maxSpawn <= std::int32_t(spawner.spawned);
        if (!spawner.params.startSpawned || limitReached) {
            hold(spawner, i, world);
            continue;
        }
        world.restore(spawner.slots[i].character);
        spawner.slots[i].countdown = spawner.params.respawnFrames;
        spawner.heldMask = std::uint16_t(spawner.heldMask & ~bit(i));
        if (spawner.params.maxOnScreen != 0) ++spawner.onScreen;
        ++spawner.spawned;
    }
}

void AiSpawnerSet::countHeld(AiSpawner& spawner, AiSpawnerWorld& world) {
    spawner.requested = AiSpawnerState::Active;
    spawner.maxSpawn = std::int16_t(spawner.spawned);
    for (const AiSpawnerSlot& slot : spawner.slots)
        if (world.held(slot.character)) ++spawner.maxSpawn;
}

bool AiSpawnerSet::message(const std::string& instance, std::uint8_t message, AiSpawnerWorld& world) {
    const auto found = std::find_if(spawners_.begin(), spawners_.end(), [&](const AiSpawner& s) { return s.instance == instance; });
    if (found == spawners_.end()) return false;
    AiSpawner& spawner = *found;
    switch (message) {
    case kAiSpawnerMessageReset: reset(spawner, world); break;
    case kAiSpawnerMessageCountHeld: countHeld(spawner, world); break;
    case kAiSpawnerMessageOff: spawner.requested = AiSpawnerState::Idle; break;
    case kAiSpawnerMessageOn:
        spawner.requested = spawner.requested == AiSpawnerState::Idle ? AiSpawnerState::Active : AiSpawnerState::Idle;
        break;
    default: break;
    }
    return true;
}

// STAND-IN SCRIPT-S322: in state 3 the original tests the bound only when (frame counter 0x241098 + entity id) % 15 is not 0; the port tests it in
// every update.
void AiSpawnerSet::preUpdate(AiSpawner& spawner, AiSpawnerWorld& world) {
    if (spawner.params.playerInsideBound.empty() || spawner.requested == AiSpawnerState::Idle) return;
    if (!world.playerInside(spawner)) {
        spawner.requested = AiSpawnerState::PlayerOutside;
        return;
    }
    if (spawner.requested == AiSpawnerState::PlayerOutside) spawner.requested = AiSpawnerState::Active;
}

void AiSpawnerSet::stateUpdate(AiSpawner& spawner, AiSpawnerWorld& world) {
    const AiSpawnerParams& params = spawner.params;
    if (spawner.state == AiSpawnerState::Active) {
        const bool allSpawned = spawner.maxSpawn != 0 && std::int32_t(spawner.spawned) == spawner.maxSpawn;
        const bool screenFull = params.maxOnScreen != 0 && std::int32_t(params.maxOnScreen) <= spawner.onScreen;
        if (!allSpawned && !screenFull) {
            for (std::size_t i = 0; i < spawner.slots.size(); ++i) {
                AiSpawnerSlot& slot = spawner.slots[i];
                if (!world.held(slot.character)) continue;
                if (slot.countdown != 0) {
                    --slot.countdown;
                    continue;
                }
                if (spawner.stagger != 0 || !world.controllerAllowsSpawn(slot.character)) continue;
                if (!params.spawnOnScreen && world.inView(params.spawnFromSpawner ? spawner.matrix : slot.placed)) continue;
                spawner.requested = AiSpawnerState::Spawn;
                spawner.spawnSlot = std::uint8_t(i);
                break;
            }
            if (spawner.stagger != 0) --spawner.stagger;
        }
    } else if (spawner.state == AiSpawnerState::Spawn) {
        if (!params.waitForDead && spawner.maxSpawn != 0 && std::int32_t(spawner.spawned) == spawner.maxSpawn) {
            world.activateReference(spawner.instance);
            events_.push_back(eventOf(AiSpawnerEventKind::Fired, spawner.instance));
            spawner.requested = AiSpawnerState::Idle;
        } else {
            spawner.requested = AiSpawnerState::Active;
        }
    }
    heldCheck(spawner, world);
}

void AiSpawnerSet::heldCheck(AiSpawner& spawner, AiSpawnerWorld& world) {
    const bool allSpawned = spawner.maxSpawn != 0 && std::int32_t(spawner.spawned) == spawner.maxSpawn;
    if (!allSpawned && spawner.params.maxOnScreen == 0) return;
    bool allHeld = std::int32_t(spawner.spawned) == spawner.maxSpawn;
    for (std::size_t i = 0; i < spawner.slots.size(); ++i) {
        if (!world.held(spawner.slots[i].character)) {
            allHeld = false;
            continue;
        }
        if (spawner.heldMask & bit(i)) continue;
        spawner.heldMask |= bit(i);
        if (spawner.params.maxOnScreen != 0) --spawner.onScreen;
        spawner.lastHeld = std::uint8_t(i);
    }
    if (!allHeld) return;
    if (spawner.revealPending && world.disabled(spawner.params.revealObject)) {
        world.reveal(spawner.params.revealObject, spawner.slots[spawner.lastHeld].character);
        events_.push_back(eventOf(AiSpawnerEventKind::Revealed, spawner.instance));
        spawner.revealPending = false;
    }
    if (spawner.params.waitForDead && world.referenceIdle(spawner.instance)) {
        world.activateReference(spawner.instance);
        events_.push_back(eventOf(AiSpawnerEventKind::Fired, spawner.instance));
        spawner.requested = AiSpawnerState::Idle;
    }
}

// STAND-IN SCRIPT-S322: the clips "close", "spawn" and "waiting" (+0x48..+0x50, and the wait for the spawn clip 0x245f84 in state 2), StartEffect,
// ATTR_SFX with its SFXPlayTime countdown and the call 0x2320f8 of state 3 are not applied; no spawner of the eight levels has a mesh, an effect or a
// sound. STAND-IN SCRIPT-S321: the spawned character's heading (ctrl+8 / +0xa), ctrl+0x314 bit 6 and the StartState entry of the table 0x58db48 (+0xc,
// filled at run time) are not applied; the character stands where it came out.
void AiSpawnerSet::stateChange(AiSpawner& spawner, AiSpawnerWorld& world) {
    if (spawner.requested == spawner.state) return;
    if (spawner.requested == AiSpawnerState::Spawn) {
        const std::size_t index = spawner.spawnSlot;
        AiSpawnerSlot& slot = spawner.slots[index];
        if (!world.release(slot.character, spawner.params.spawnFromSpawner ? spawner.matrix : slot.placed)) return;
        spawner.heldMask = std::uint16_t(spawner.heldMask & ~bit(index));
        if (spawner.params.maxOnScreen != 0) ++spawner.onScreen;
        world.startState(spawner, index);
        world.aiSpawned(spawner.instance);
        world.aiSpawned(slot.character);
        spawner.stagger = spawner.params.staggerFrames;
        slot.countdown = spawner.params.respawnFrames;
        ++spawner.spawned;
        AiSpawnerEvent spawned = eventOf(AiSpawnerEventKind::Spawned, spawner.instance);
        spawned.character = slot.character;
        spawned.slot = std::uint8_t(index);
        spawned.spawned = spawner.spawned;
        spawned.maxSpawn = std::uint16_t(spawner.maxSpawn);
        events_.push_back(std::move(spawned));
    }
    spawner.state = spawner.requested;
    AiSpawnerEvent changed = eventOf(AiSpawnerEventKind::State, spawner.instance);
    changed.state = spawner.state;
    events_.push_back(std::move(changed));
}

// The gate of 0x1a5d2c, +0x60 bit 3 (written by 0x1988dc, caller not read), is never set in the port.
void AiSpawnerSet::update(AiSpawnerWorld& world) {
    for (AiSpawner& spawner : spawners_) {
        preUpdate(spawner, world);
        if (spawner.state == spawner.requested) stateUpdate(spawner, world);
        stateChange(spawner, world);
    }
}

std::vector<AiSpawnerEvent> AiSpawnerSet::takeEvents() {
    std::vector<AiSpawnerEvent> taken;
    taken.swap(events_);
    return taken;
}

}
