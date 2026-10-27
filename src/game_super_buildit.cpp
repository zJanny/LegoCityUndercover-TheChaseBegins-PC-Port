#include "frontend/pages.hpp"
#include "frontend/super_build_prompt.hpp"
#include "game_level.hpp"
#include "runtime/superbuildit_messages.hpp"
#include <algorithm>
#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;
namespace fe = chase::frontend;

namespace {
constexpr const char* kGameText = "localisation/gametext_uk.loc";
constexpr const char* kHintText = "localisation/hinttext_uk.loc";
constexpr const char* kSystemText = "localisation/systemtext_uk.loc";
constexpr const char* kSuperBuilditTable = "tables/tablessuperbuildit.bxls";
constexpr const char* kMissingString = "MISSING STRING";  // 0x235fb4
}

void LevelSession::loadSuperBuildits() {
    superBuildits.load(shared.archive, level, classOf);
    runtime::applySuperBuilditLink(superBuildits, objectState);
    std::vector<runtime::SuperBuilditRecord> saved;
    for (const chase::save::ObjectRecord& record : saveLevelState.keptRecords)
        if (record.payload.size() == 1) saved.push_back({record.hash, record.payload[0]});
    const std::size_t restored = runtime::restoreSuperBuildits(superBuildits, saved, objectState, levelScripts);
    if (restored) std::cout << "super build-its restored from the saved state: " << restored << '\n';
    saveLevelState.liveRecords.clear();
    for (const runtime::SuperBuilditRecord& record : runtime::superBuilditRecords(superBuildits)) saveLevelState.liveRecords.push_back({record.hash, {record.payload}});
    if (const chase::assets::FuseEntry* entry = shared.archive.find(kSuperBuilditTable)) {
        const chase::assets::Table table = chase::assets::parseTable(shared.archive.read(*entry));
        if (!table.sheets.empty())
            for (unsigned row = 0; row < table.sheets[0].rows; ++row) superBuild.pictures.push_back(chase::assets::tableValue(table.sheets[0], row, 1).text);
    }
    for (const runtime::SuperBuilditObject& plate : superBuildits.objects()) {
        if (plate.parent.empty()) continue;
        const std::optional<chase::assets::Placement> holder = chase::assets::objectStart(level, plate.parent);
        const chase::assets::Level* holderClass = nullptr;
        for (const chase::assets::LevelObject& object : level.objects)
            if (game_doors::lowerCase(object.instance) == plate.parent) holderClass = classOf(object.className);
        if (!holder || !holderClass) continue;
        for (const chase::assets::Placement& child : chase::assets::objectPlacements(*holderClass, classOf)) {
            const std::string name = plate.parent + "." + game_doors::lowerCase(child.instance);
            if (name != plate.instance && name != plate.propToReveal) continue;
            const std::size_t index = scene.instances();
            if (!scene.addModel(chase::assets::meshModelPath(child.mesh),
                                chase::assets::multiply(chase::assets::placementMatrix(child), chase::assets::placementMatrix(*holder))))
                continue;
            superBuild.models.push_back({name, plate.parent, index});
        }
    }
    std::cout << "super build-its: " << superBuildits.objects().size() << '\n';
}

// STAND-IN HUD-S78: no HUD update runs while the dialog or the movie holds the game, so the count display's update count and spin stand still.
bool LevelSession::holdForSuperBuildit(float dt) {
    if (superBuildits.confirmOpen()) {
        if (pad.slot(kSlotConfirmButton).press > 0) superBuildits.answer(runtime::kSuperBuilditDialogConfirm);
        else if (pad.slot(kSlotCancelButton).press > 0) superBuildits.answer(runtime::kSuperBuilditDialogCancel);
        if (!superBuildits.confirmOpen()) std::cout << "super build-it dialog answered at update " << updates << '\n';
    } else if (superBuild.moviePlaying) {
        superBuild.moviePlaying = movies->advance(dt);
        if (!superBuild.moviePlaying) std::cout << "super build-it movie ended at update " << updates << '\n';
    } else {
        return false;
    }
    ++updates;
    ++updatesThisSecond;
    return true;
}

void LevelSession::deliverSuperBuilditMessage(const std::string& instance, std::uint8_t message) {
    runtime::SuperBuilditObject* object = superBuildits.findMutable(instance);
    if (!object) return;
    const runtime::SuperBuilditMessageResult result = runtime::superBuilditMessage(
        *object, message, objectState, [this](const std::string& name) { return levelScripts.variable(chase::assets::nameHash(name)); });
    if (result.handled) std::cout << "super build-it " << instance << ": message " << unsigned(message) << " at update " << updates << '\n';
}

void LevelSession::updateSuperBuilditSaves() {
    const std::vector<chase::runtime::ObjectSuperBuiltItSave>& saves = levelScripts.progressNatives().superBuiltItSaves();
    for (; superBuild.savesApplied < saves.size(); ++superBuild.savesApplied) {
        const chase::runtime::ObjectSuperBuiltItSave& save = saves[superBuild.savesApplied];
        if (runtime::SuperBuilditObject* object = superBuildits.findMutable(save.instance)) runtime::disableSuperBuilditSave(*object, save.disabled);
    }
    for (const runtime::ObjectClassMessage& sent : levelScripts.takeObjectClassMessagesIf(
             [this](const runtime::ObjectClassMessage& message) { return message.message == runtime::kMessageReset && superBuildits.find(message.instance); }))
        deliverSuperBuilditMessage(sent.instance, sent.message);
    saveLevelState.liveRecords.clear();
    for (const runtime::SuperBuilditRecord& record : runtime::superBuilditRecords(superBuildits)) saveLevelState.liveRecords.push_back({record.hash, {record.payload}});
}

void LevelSession::updateSuperBuildits(const float playerPosition[3], float dt) {
    updateSuperBuilditSaves();
    runtime::SuperBuilditInput input;
    std::copy_n(playerPosition, 3, input.player);
    input.playerCarrying = carry.carryingMode() != 0 || carry.busy();
    input.playerFree = !input.playerCarrying && !levelScripts.playerControlTaken() && !cutscenePlaying() && !builditPlayer.active() &&
                       player->vertical.posture == runtime::Posture::Ground && !(boarding && boarding->seatedVehicle());
    input.actionHeld = pad.slot(kSlotConfirmButton).value > 0.0f;
    input.superBricks = superBricks.total();
    const std::vector<runtime::SuperBuilditAction> actions = superBuildits.update(objectState, input);
    const runtime::SuperBuilditSpend spend = runtime::applySuperBuilditActions(superBuildits, actions, objectState, levelScripts);
    superBricks.spend(spend.bricks);
    if (hud) hud->superBrickSpent(spend.bricks);
    for (unsigned i = 0; i < spend.saveRequests; ++i) levelScripts.progressNatives().requestSave(superBricks.total());
    superBuild.promptTime += dt;
    for (const runtime::SuperBuilditAction& action : actions) {
        const runtime::SuperBuilditObject& object = superBuildits.objects()[action.index];
        switch (action.kind) {
        case runtime::SuperBuilditActionKind::PromptShown:
            superBuild.promptOn = true;
            superBuild.promptTime = 0;
            std::cout << "super build-it " << object.instance << ": prompt shown at update " << updates << " (needs " << object.value << ", has "
                      << input.superBricks << ")\n";
            break;
        case runtime::SuperBuilditActionKind::PromptHidden:
            superBuild.promptOn = false;
            superBuild.promptTime = 0;
            std::cout << "super build-it " << object.instance << ": prompt hidden at update " << updates << '\n';
            break;
        case runtime::SuperBuilditActionKind::ConfirmOpened:
            std::cout << "super build-it " << object.instance << ": dialog opened at update " << updates << '\n';
            break;
        case runtime::SuperBuilditActionKind::MovieStarted:
            superBuild.moviePlaying = !object.fmv.empty() && movies->open(game_doors::lowerCase(object.fmv)) && movies->advance(dt);
            std::cout << "super build-it " << object.instance << ": movie " << object.fmv << (superBuild.moviePlaying ? "" : " (not found)") << " at update " << updates
                      << '\n';
            break;
        case runtime::SuperBuilditActionKind::Built:
            if (object.soundBuilt && player)
                player->sounds.push_back({object.soundBuilt, {object.world[12], object.world[13], object.world[14]}, levelScripts.soundNatives().ownerOf(object.instance)});
            std::cout << "super build-it " << object.instance << ": built at update " << updates << ", super bricks " << superBricks.total() << ", shown count "
                      << (hud ? hud->superBrickDisplay().count() : 0) << '\n';
            break;
        }
    }
    if (hud && superBuild.promptOn) hud->superBrickPickedUp();
    for (const game_super_buildit::Model& model : superBuild.models)
        scene.setInstanceVisible(model.sceneIndex, !objectState.isDisabled(model.instance) && !objectState.isDisabled(model.parent));
}

void LevelSession::drawSuperBuildit(fe::Canvas& top) {
    const std::optional<runtime::SuperBuilditPrompt>& prompt = superBuildits.prompt();
    if (prompt) drawSuperBuilditPrompt(top, *prompt);
    if (superBuildits.confirmOpen())
        fe::drawWarningDialog(top, hudResources, hudResources.textForKey(kGameText, runtime::kSuperBuilditConfirmBodyKey),
                              hudResources.textForKey(kSystemText, runtime::kSuperBuilditConfirmOkKey),
                              hudResources.textForKey(kSystemText, runtime::kSuperBuilditConfirmCancelKey));
}

void LevelSession::drawSuperBuilditPrompt(fe::Canvas& top, const runtime::SuperBuilditPrompt& prompt) {
    const runtime::SuperBuilditObject& object = superBuildits.objects()[prompt.object];
    fe::SuperBuildPromptView view;
    view.name = hudResources.textForKey(kGameText, object.textPopup);
    if (view.name.empty()) view.name = hudResources.textForKey(kHintText, object.textPopup);
    if (view.name.empty()) view.name = kMissingString;
    view.cost = prompt.value == 0 ? hudResources.textForKey(kGameText, runtime::kSuperBuilditFreeKey) : std::to_string(prompt.value);
    view.prompt = hudResources.textForKey(kGameText, prompt.enough ? runtime::kSuperBuilditBuildKey : runtime::kSuperBuilditNotEnoughKey);
    view.picture = runtime::superBuilditPicture(superBuild.pictures, prompt.type);
    view.enough = prompt.enough;
    view.on = superBuild.promptOn;
    view.time = superBuild.promptTime;
    fe::drawSuperBuildPrompt(top, hudResources, view);
}

}
