#include "game_level.hpp"
#include "frontend/map_marker.hpp"
#include "runtime/sub_objectives.hpp"

namespace game_level {
namespace runtime = chase::runtime;
namespace fe = chase::frontend;

namespace {
// 0x2edb4c: "Sprites/Game/" + SubObjectiveArrowGoto, ...Enemy, ...Carry, the textures of the lists 0, 1, 2.
constexpr std::array<const char*, runtime::kObjectiveArrowLists> kArrowTextures = {"subobjectivearrowgoto", "subobjectivearrowenemy", "subobjectivearrowcarry"};
// STAND-IN (SCRIPT-S270): the entity box (+0x60 / +0x6c) of characters is the player's oracle value (centre y 1.15, half y 0.85,
// docs/re/collision.md); props use the box of their collision entry and other entities 0.
constexpr float kCharacterBoxTop = 1.15f + 0.85f;
}

void LevelSession::loadObjectiveBoxes(const std::vector<chase::assets::ObjectCollision>& placed) {
    objectiveBoxTops.clear();
    for (const chase::assets::ObjectCollision& object : placed) objectiveBoxTops[game_doors::lowerCase(object.instance)] = object.centre[1] + object.half[1];
}

bool LevelSession::objectiveTargetPosition(const std::string& instance, float out[3]) const {
    if (player && instance == "player1") {
        out[0] = player->motion.x;
        out[1] = player->vertical.y;
        out[2] = player->motion.z;
        return true;
    }
    for (const runtime::BoardingVehicle& vehicle : boardingVehicles)
        if (vehicle.instance == instance) {
            for (int axis = 0; axis < 3; ++axis) out[axis] = vehicle.matrix[std::size_t(12 + axis)];
            return true;
        }
    if (const runtime::AiCharacter* character = levelScripts.ai().character(instance))
        if (character->position) {
            for (int axis = 0; axis < 3; ++axis) out[axis] = (*character->position)[std::size_t(axis)];
            return true;
        }
    if (carry.position(instance, out)) return true;
    const std::optional<chase::assets::Placement> placement = objectStart(level, instance);
    if (!placement) return false;
    std::copy(placement->position, placement->position + 3, out);
    return true;
}

bool LevelSession::objectiveUseMarker(const std::string& instance, float out[3], float& alpha) const {
    if (!useMarkers.slotted(instance)) return false;
    for (const runtime::UseSpot& spot : useSpots.spots()) {
        if (spot.instance != instance) continue;
        for (int a = 0; a < 3; ++a)
            out[a] = spot.matrix[std::size_t(12 + a)] + spot.zoneCentre[0] * spot.matrix[std::size_t(a)] +
                     spot.zoneCentre[1] * spot.matrix[std::size_t(4 + a)] + spot.zoneCentre[2] * spot.matrix[std::size_t(8 + a)];
        alpha = useMarkers.alpha(instance);
        return true;
    }
    return false;
}

void LevelSession::updateObjectiveMarkers(float dt) {
    if (!player || !follow) return;
    objectiveArrowClock += dt;
    runtime::SubObjectiveWorld world;
    world.player = {player->motion.x, player->vertical.y, player->motion.z};
    const float playerX = player->motion.x, playerZ = player->motion.z, yaw = follow->yaw();
    world.onMap = [playerX, playerZ, yaw](const float position[3]) { return fe::mapPositionOnMap(playerX, playerZ, yaw, position[0], position[2]); };
    world.position = [this](const std::string& instance, float out[3]) { return objectiveTargetPosition(instance, out); };
    world.useMarker = [this](const std::string& instance, float out[3], float& alpha) { return objectiveUseMarker(instance, out, alpha); };
    world.keyIsManagerEntity = [this](const std::string& key) { return levelScripts.waypoints().isCarryItWaypoint(game_doors::lowerCase(key)); };
    levelScripts.updateObjectiveMarkers(dt, world);
}

void LevelSession::addObjectiveMapIcons(fe::MinimapView& view) const {
    for (std::uint8_t type = 0; type < runtime::kSubObjectiveSlotIcons.size(); ++type)
        for (const runtime::SubObjectiveGroup& group : levelScripts.objectives().groups()) {
            if (group.type != type) continue;
            const int icon = runtime::kSubObjectiveSlotIcons[type];
            const float slotFade = group.hasSlot ? group.slotTimer / runtime::kSubObjectiveSlotFadeSeconds : 0.0f;
            fe::MinimapGroupTargets targets;
            targets.iconType = icon;
            targets.alpha = 1.0f - slotFade;
            for (const std::string& target : group.targets) {
                float position[3];
                if (!objectState.isHidden(target) && objectiveTargetPosition(target, position)) targets.targets.push_back({position[0], position[2]});
            }
            view.groupTargets.push_back(std::move(targets));
            if (group.hasSlot) view.groupSlots.push_back({icon, group.centroid[0], group.centroid[2], slotFade});
        }
}

void LevelSession::drawObjectiveArrows(const chase::render::Mat4& viewProjection, const chase::render::Mat4& viewToWorld) {
    if (!player) return;
    const float right[3] = {viewToWorld[0], viewToWorld[1], viewToWorld[2]}, up[3] = {viewToWorld[4], viewToWorld[5], viewToWorld[6]};
    std::vector<runtime::ParticleBatch> batches;
    for (std::size_t list = 0; list < runtime::kObjectiveArrowLists; ++list)
        for (const runtime::ObjectiveArrow& arrow : levelScripts.objectives().arrows()[list]) {
            std::array<float, 3> anchor;
            int baseAlpha = 255;
            float point[3], markerAlpha = 0;
            if (objectiveUseMarker(arrow.target, point, markerAlpha)) {
                anchor = runtime::objectiveArrowUseMarkerAnchor(point, objectiveArrowClock);
                baseAlpha = int(markerAlpha * 255.0f);
            } else {
                if (!objectiveTargetPosition(arrow.target, point)) continue;
                const auto box = objectiveBoxTops.find(arrow.target);
                const bool character = arrow.target == "player1" || levelScripts.ai().character(arrow.target) != nullptr;
                const float boxTop = box != objectiveBoxTops.end() ? box->second : (character ? kCharacterBoxTop : 0.0f);
                anchor = runtime::objectiveArrowAnchor(point, boxTop, 0.0f, objectiveArrowClock);
            }
            const int alpha = runtime::objectiveArrowAlphaByte(arrow, baseAlpha);
            if (alpha <= 0) continue;
            const std::array<std::array<float, 3>, 4> corners = runtime::objectiveArrowQuad(anchor, right, up);
            constexpr float uvs[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
            runtime::ParticleBatch batch;
            batch.texture = int(list);
            for (int corner = 0; corner < 4; ++corner)
                batch.vertices.push_back({{corners[std::size_t(corner)][0], corners[std::size_t(corner)][1], corners[std::size_t(corner)][2]},
                                          {uvs[corner][0], uvs[corner][1]},
                                          {255, 255, 255, std::uint8_t(alpha)}});
            batches.push_back(std::move(batch));
        }
    if (batches.empty()) return;
    if (!objectiveArrowRenderer) objectiveArrowRenderer.emplace(shared.archive, std::vector<std::string>(kArrowTextures.begin(), kArrowTextures.end()), "sprites/game/");
    objectiveArrowRenderer->draw(viewProjection, batches);
}

}
