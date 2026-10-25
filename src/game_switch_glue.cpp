#include "game_level.hpp"
#include "runtime/prop_animation.hpp"
#include "runtime/stud_drop_classes.hpp"

#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

void LevelSession::loadSwitches(chase::assets::FuseArchive& archive) {
    std::map<std::string, std::string> meshes;
    for (const chase::assets::Placement& placed : placedObjects) meshes[placed.instance] = placed.mesh;
    switches.load(level, classOf, [&](const chase::assets::LevelObject& object, const chase::assets::Level&) {
        const auto mesh = meshes.find(object.instance);
        return mesh != meshes.end() && !mesh->second.empty() && archive.find(runtime::PropAnimations::animationPath(mesh->second, "use")) != nullptr;
    });
    pendingSwitchEvents = switches.link(buildits.references());
    std::cout << "switches: " << switches.objects().size() << '\n';
}

void LevelSession::useSwitches(bool actionFree) {
    runtime::SwitchUseInput input;
    input.player[0] = player->motion.x;
    input.player[1] = player->vertical.y;
    input.player[2] = player->motion.z;
    input.actionPressed = pad.slot(kSlotConfirmButton).press > 0;
    input.playerFree = actionFree && !levelScripts.playerControlTaken() && !cutscenePlaying() && !detective.controlTaken() &&
                       player->vertical.posture == runtime::Posture::Ground;
    input.playerHasAbility = [this](int ability) { return abilityTable.characterHas(game_player::kPlayerCharacterId, ability); };
    if (const auto used = switches.use(objectState, input, buildits.references()))
        std::cout << "switch " << switches.objects()[used->index].instance << (used->answer == 1 ? ": used" : ": refused for the abilities") << " at update "
                  << updates << '\n';
}

void LevelSession::deliverSwitchMessage(const runtime::ScriptObjectMessage& sent) {
    switches.message(sent.instance, sent.message, buildits.references(), levelScripts, pendingSwitchEvents);
}

void LevelSession::updateSwitches(float dt, const std::function<void(const runtime::SoundCall&)>& emit) {
    std::vector<runtime::SwitchState> before;
    for (const runtime::SwitchObject& object : switches.objects()) before.push_back(object.record.state);
    std::vector<runtime::SwitchEvent> events = std::move(pendingSwitchEvents);
    pendingSwitchEvents.clear();
    for (runtime::SwitchEvent& event : switches.update(objectState, dt, buildits.references(), levelScripts)) events.push_back(event);
    for (std::size_t i = 0; i < switches.objects().size(); ++i)
        if (switches.objects()[i].record.state != before[i])
            std::cout << "switch " << switches.objects()[i].instance << ": state " << unsigned(switches.objects()[i].record.state) << " at update " << updates << '\n';
    for (const runtime::SwitchEvent& event : events) {
        const runtime::SwitchObject& object = switches.objects()[event.index];
        switch (event.effect.kind) {
        case runtime::SwitchEffectKind::UseSound:
        case runtime::SwitchEffectKind::SoundOn:
        case runtime::SwitchEffectKind::SoundOff:
            emit({event.effect.sound, {object.position[0], object.position[1], object.position[2]}, levelScripts.soundNatives().ownerOf(object.instance)});
            break;
        case runtime::SwitchEffectKind::StopUseSound:
            if (shared.audio) shared.audio->stop(event.effect.sound, levelScripts.soundNatives().ownerOf(object.instance));
            break;
        case runtime::SwitchEffectKind::StudDrop:
            if (const auto mode = studDrops.drop(runtime::genericStateStudRequest(object.studSpawn, object.studValue, object.matrix), debrisLineStandIn(),
                                                 debrisLineStandIn()))
                std::cout << "switch " << object.instance << ": stud drop mode " << unsigned(*mode) << '\n';
            break;
        default: break;
        }
    }
}

}
