#include "game_level.hpp"
#include "game_super_bricks.hpp"

#include <sstream>

namespace game_super_bricks {
namespace runtime = chase::runtime;

void LevelSuperBricks::load(const std::vector<chase::assets::SuperBrickObject>& bricks, const std::vector<chase::assets::Placement>& placedObjects) {
    for (const chase::assets::SuperBrickObject& brick : bricks) {
        field_.add(brick.instance, {brick.position[0], brick.position[1], brick.position[2]}, brick.value);
        std::size_t index = kNoModel;
        for (std::size_t i = 0; i < placedObjects.size(); ++i)
            if (placedObjects[i].instance == brick.instance) index = i;
        placedIndex_.push_back(index);
        spinMatrix_.push_back(runtime::superBrickMatrix({brick.position[0], brick.position[1], brick.position[2]}, 0));
    }
}

Result LevelSuperBricks::update(float dt, const std::array<float, 3>& playerPosition, const std::function<bool(const std::string&)>& disabled,
                                const std::optional<std::array<float, 3>>& flightTarget) {
    Result result;
    tint_.update(dt);
    const runtime::SuperBrickStep step =
        field_.update(dt, playerPosition, false, [&](std::size_t i) { return !disabled(field_.bricks()[i].instance); }, flightTarget);
    result.pickups = unsigned(step.collected.size());
    for (const runtime::SuperBrickBanked& banked : step.banked) {
        result.totals.push_back(banked.total);
        result.sounds.push_back(banked.sound);
        ++result.saveRequests;
    }
    return result;
}

void LevelSuperBricks::apply(chase::render::Scene& scene, const std::vector<std::size_t>& placedSceneIndex,
                             const std::function<bool(const std::string&)>& disabled, std::uint32_t clockTicks) {
    for (std::size_t i = 0; i < field_.bricks().size(); ++i) {
        if (placedIndex_[i] == kNoModel || placedIndex_[i] >= placedSceneIndex.size()) continue;
        const runtime::SuperBrick& brick = field_.bricks()[i];
        const std::size_t instance = placedSceneIndex[placedIndex_[i]];
        if (brick.state == runtime::SuperBrickState::Idle) {
            if (!disabled(brick.instance)) {
                spinMatrix_[i] = runtime::superBrickMatrix(brick.position, clockTicks);
                scene.setTransform(instance, spinMatrix_[i]);
                scene.setInstanceTint(instance, tint_.colour());
            }
        } else if (brick.state == runtime::SuperBrickState::Collected) {
            std::array<float, 16> matrix = spinMatrix_[i];
            for (std::size_t axis = 0; axis < 3; ++axis) matrix[12 + axis] = brick.position[axis];
            scene.setTransform(instance, matrix);
            scene.setInstanceFade(instance, runtime::superBrickAlpha(brick.timer));
        } else {
            scene.setInstanceFade(instance, 0.0f);
        }
    }
}

std::string LevelSuperBricks::report() const {
    std::ostringstream text;
    text << "super bricks: total " << field_.total() << ", " << field_.count(runtime::SuperBrickState::Collected) << " collecting, "
         << field_.count(runtime::SuperBrickState::Done) << " collected, " << field_.count(runtime::SuperBrickState::Idle) << " left";
    return text.str();
}

}

namespace game_level {
namespace runtime = chase::runtime;

void LevelSession::updateSuperBricks(float dt) {
    const std::array<float, 3> playerPosition = {player->motion.x, player->vertical.y, player->motion.z};
    if (hud) hud->setSuperBrickCount(superBricks.total());
    const std::optional<std::array<float, 3>> flightTarget = runtime::unprojectTopScreen(
        200.0f, 120.0f, runtime::kSuperBrickFlightDepth, follow->eye(), follow->yaw(), follow->pitch(), follow->verticalFieldOfView());
    const game_super_bricks::Result result = superBricks.update(
        dt, playerPosition, [this](const std::string& instance) { return objectState.isDisabled(instance); }, flightTarget);
    for (const std::uint16_t sound : result.sounds) {
        soundLog.emplace_back(updates, sound);
        if (shared.audio) shared.audio->playUi(sound);
    }
    if (hud) {
        for (unsigned i = 0; i < result.pickups; ++i) hud->superBrickPickedUp();
        for (const std::uint32_t total : result.totals) hud->superBrickBanked(total);
    }
    for (unsigned i = 0; i < result.saveRequests; ++i) levelScripts.progressNatives().requestSave(superBricks.total());
}

void LevelSession::applySuperBricks() {
    superBricks.apply(scene, placedSceneIndex, [this](const std::string& instance) { return objectState.isDisabled(instance); },
                      std::uint32_t(provisionalStudClockSeconds * double(runtime::kSuperBrickClockTicksPerSecond)));
}

}
