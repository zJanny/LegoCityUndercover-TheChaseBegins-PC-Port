#include "game_level.hpp"

#include "runtime/reference_trigger_apply.hpp"

#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

namespace {

class SessionSpawnerWorld : public runtime::AiSpawnerWorld {
public:
    using Place = std::function<bool(const std::string&, const chase::assets::Matrix&)>;
    SessionSpawnerWorld(chase::assets::PlacedObjectState& state, runtime::ReferenceTriggers& references, runtime::ScriptRunner& scripts, Place place)
        : state_(state), references_(references), scripts_(scripts), place_(std::move(place)) {}

    bool held(const std::string& character) const override { return state_.destroyed.count(character) != 0; }
    bool disabled(const std::string& object) const override { return state_.disabled.count(object) != 0; }
    // STAND-IN SCRIPT-S321: message 53 (0x236724, the ability items of the character's controller) has no counterpart for level characters.
    void hold(const std::string& character) override {
        state_.destroyed.insert(character);
        state_.disabled.insert(character);
    }
    void disable(const std::string& object) override { state_.disabled.insert(object); }
    void restore(const std::string& character) override {
        state_.destroyed.erase(character);
        state_.disabled.erase(character);
    }
    // STAND-IN SCRIPT-S321: the controller tests of 0x288fbc (+0x16c, requested state +0x6e 11) and of 0x268978 (+0x6e 1 after message 52) pass; the
    // reset 0x218b88 of message 52 and message 2 are not applied.
    bool controllerAllowsSpawn(const std::string&) const override { return true; }
    bool release(const std::string& character, const chase::assets::Matrix& matrix) override {
        place_(character, matrix);
        state_.disabled.erase(character);
        state_.destroyed.erase(character);
        return true;
    }
    // STAND-IN SCRIPT-S322: the view test 0x265190 is not ported, so the 24 spawners with SpawnOnScreen 0 spawn as if the point were out of view; the
    // bound test answers "inside" (no spawner of the eight levels sets PlayerInsideBound).
    bool inView(const chase::assets::Matrix&) const override { return false; }
    bool playerInside(const runtime::AiSpawner&) const override { return true; }
    void aiSpawned(const std::string& object) override { scripts_.fireObjectEvent("aispawned", object); }
    void activateReference(const std::string& spawner) override {
        if (const auto id = references_.findOwner(spawner)) runtime::applyReferenceActivation(references_.activate(*id), scripts_);
    }
    void deactivateReference(const std::string& spawner) override {
        if (const auto id = references_.findOwner(spawner)) runtime::applyReferenceActivation(references_.deactivate(*id), scripts_);
    }
    bool referenceIdle(const std::string& spawner) const override {
        const auto id = references_.findOwner(spawner);
        return !id || references_.kind(*id) == runtime::ReferenceKind::Idle;
    }
    // STAND-IN SCRIPT-S322: the reveal object is only enabled; its move to the last held character and message 51 are not applied (no spawner of the
    // eight levels sets RevealObject).
    void reveal(const std::string& object, const std::string&) override { state_.disabled.erase(object); }

private:
    chase::assets::PlacedObjectState& state_;
    runtime::ReferenceTriggers& references_;
    runtime::ScriptRunner& scripts_;
    Place place_;
};

}

void LevelSession::withAiSpawnerWorld(const std::function<void(runtime::AiSpawnerWorld&)>& use) {
    SessionSpawnerWorld world(objectState, buildits.references(), levelScripts, [this](const std::string& character, const chase::assets::Matrix& matrix) {
        return levelCharacters && levelCharacters->place(character, matrix);
    });
    use(world);
}

void LevelSession::loadAiSpawners() {
    aiSpawners.load(level, classOf, [this](const std::string& character) -> std::optional<chase::assets::Matrix> {
        if (levelCharacters) return levelCharacters->world(character);
        return std::nullopt;
    });
    withAiSpawnerWorld([this](runtime::AiSpawnerWorld& world) { aiSpawners.postLink(world); });
    std::size_t held = 0;
    for (const runtime::AiSpawner& spawner : aiSpawners.spawners())
        for (const runtime::AiSpawnerSlot& slot : spawner.slots) held += objectState.destroyed.count(slot.character);
    std::cout << "ai spawners: " << aiSpawners.spawners().size() << ", " << held << " characters held\n";
}

void LevelSession::deliverAiSpawnerMessage(const runtime::ScriptObjectMessage& sent) {
    withAiSpawnerWorld([&](runtime::AiSpawnerWorld& world) { aiSpawners.message(sent.instance, sent.message, world); });
}

void LevelSession::updateAiSpawners() {
    withAiSpawnerWorld([this](runtime::AiSpawnerWorld& world) { aiSpawners.update(world); });
    for (const runtime::AiSpawnerEvent& event : aiSpawners.takeEvents()) {
        std::cout << "ai spawner " << event.spawner << ": ";
        switch (event.kind) {
        case runtime::AiSpawnerEventKind::State:
            std::cout << "state " << unsigned(event.state) << ' ' << runtime::aiSpawnerStateName(event.state);
            break;
        case runtime::AiSpawnerEventKind::Spawned:
            std::cout << "spawned " << event.character << " (slot " << unsigned(event.slot) << ", " << event.spawned << '/' << event.maxSpawn << ')';
            break;
        case runtime::AiSpawnerEventKind::Fired: {
            const auto id = buildits.references().findOwner(event.spawner);
            std::cout << "TriggerObject " << (id && !buildits.references().target(*id).empty() ? buildits.references().target(*id) : std::string("(none)"));
            break;
        }
        case runtime::AiSpawnerEventKind::Revealed: std::cout << "revealed"; break;
        }
        std::cout << " at update " << updatesForLog << '\n';
    }
}

}
