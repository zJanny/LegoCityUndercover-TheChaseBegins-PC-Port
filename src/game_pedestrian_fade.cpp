#include "game_pedestrian_fade.hpp"
#include "qol/view_distance.hpp"

#include <cmath>

namespace game_pedestrian_fade {

runtime::PedestrianNodeBounds nodeBoundsOf(assets::FuseArchive& archive, const std::string& bodyModelPath) {
    runtime::PedestrianNodeBounds bounds;
    float centre[3] = {0, 0, 0}, half[3] = {0, 0, 0};
    if (!assets::modelBounds(archive, bodyModelPath, centre, half)) return bounds;
    bounds.centre = {centre[0], centre[1], centre[2]};
    bounds.radius = std::sqrt(half[0] * half[0] + half[1] * half[1] + half[2] * half[2]);
    return bounds;
}

void SlotFades::load(runtime::PedestrianSystem& system, const std::vector<runtime::PedestrianNodeBounds>& bounds) {
    std::vector<runtime::Pedestrian>& pedestrians = system.manager().pedestrians();
    fades_.load(pedestrians.size());
    for (std::size_t slot = 0; slot < pedestrians.size() && slot < bounds.size(); ++slot) {
        pedestrians[slot].node = runtime::pedestrianMeshNode(bounds[slot].radius);
        fades_.setBounds(slot, bounds[slot]);
    }
}

void SlotFades::spawned(const runtime::PedestrianSystem& system, std::size_t slot) {
    if (system.manager().pedestrians()[slot].fadeStarted) fades_.start(slot);
}

void SlotFades::step(const runtime::PedestrianSystem& system, const std::vector<bool>& live, const runtime::PedestrianVec3& fadePoint) {
    runtime::PedestrianFadeInputs inputs;
    inputs.fadePoint = fadePoint;
    inputs.table = qol::withViewDistance(table_);
    const std::vector<runtime::Pedestrian>& pedestrians = system.manager().pedestrians();
    for (std::size_t slot = 0; slot < pedestrians.size() && slot < live.size(); ++slot)
        if (live[slot]) fades_.step(slot, pedestrians[slot].node, system.body(slot).node, inputs);
}

std::optional<runtime::PedestrianDrawTest> SlotFades::drawTest(const runtime::PedestrianSystem& system, std::size_t slot,
                                                               const runtime::PedestrianVec3& fadePoint) const {
    return runtime::pedestrianDrawTest(system.manager().pedestrians()[slot].node, fadePoint, qol::withViewDistance(table_));
}

void applyOpacity(render::Scene& scene, const render::CharacterInstance& instance, float opacity) {
    const auto apply = [&](std::size_t index) { scene.setInstanceFade(index, opacity); };
    apply(instance.bodyInstance);
    apply(instance.headInstance);
    if (instance.hatInstance) apply(*instance.hatInstance);
}

}
