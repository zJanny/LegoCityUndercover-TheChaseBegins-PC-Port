#include "game_prop_parts.hpp"
#include "game_doors.hpp"

namespace game_prop_parts {
using namespace chase;

void PropParts::addWaypointChildren(render::Scene& scene, const assets::ClassLookup& classOf, std::size_t waypointIndex,
                                    const assets::Placement& waypoint) {
    const assets::Level* classFile = classOf(waypoint.className);
    if (!classFile) return;
    const runtime::NodeMatrix parent = assets::placementMatrix(waypoint);
    for (const runtime::PropChild& child : runtime::propChildren(*classFile, classOf)) {
        const runtime::NodeMatrix local = assets::placementMatrix(child.local);
        const std::size_t instance = scene.instances();
        if (!scene.addModel(assets::meshModelPath(child.local.mesh), assets::multiply(local, parent))) continue;
        children_.push_back({waypointIndex, instance, parent, local, child.bouncer, runtime::MeshBouncer(local, child.bouncerParams)});
    }
}

void PropParts::loadBuildits(render::Scene& scene, assets::FuseArchive& archive, const runtime::BuilditSet& buildits,
                             const std::vector<assets::Placement>& placed, const std::vector<std::size_t>& placedScene) {
    for (std::size_t b = 0; b < buildits.objects().size(); ++b) {
        const runtime::BuilditObject& object = buildits.objects()[b];
        std::ptrdiff_t partsIndex = -1, finalIndex = -1;
        for (std::size_t i = 0; i < placed.size(); ++i) {
            const std::string name = game_doors::lowerCase(placed[i].instance);
            if (name == object.instance) partsIndex = std::ptrdiff_t(i);
            if (!object.finalProp.empty() && name == object.finalProp) finalIndex = std::ptrdiff_t(i);
        }
        if (partsIndex < 0) continue;
        const assets::Placement& parts = placed[std::size_t(partsIndex)];
        Parts entry{b, std::size_t(partsIndex), finalIndex, runtime::BuilditBricks::load(archive, parts.mesh), {}, {}};
        if (finalIndex >= 0)
            entry.bricks.buildAt(runtime::nodeRelativeTo(assets::placementMatrix(placed[std::size_t(finalIndex)]), assets::placementMatrix(parts)));
        entry.nodeInstance = scene.addModelNodes(assets::meshModelPath(parts.mesh), assets::placementMatrix(parts));
        if (entry.nodeInstance.empty()) continue;
        scene.setInstanceVisible(placedScene[std::size_t(partsIndex)], false);
        parts_.push_back(std::move(entry));
    }
}

void PropParts::step(float dt) {
    for (Child& child : children_)
        if (child.bouncer) child.motion.update(dt);
}

void PropParts::apply(render::Scene& scene, const runtime::BuilditSet& buildits, const std::vector<assets::Placement>& placed,
                      const std::vector<std::size_t>& placedScene, const std::vector<bool>& shown, const std::vector<bool>& waypointShown,
                      const std::vector<float>& badgeFade) const {
    for (const Child& child : children_) {
        scene.setInstanceVisible(child.instance, child.waypoint < waypointShown.size() && waypointShown[child.waypoint]);
        scene.setInstanceFade(child.instance, child.waypoint < badgeFade.size() ? badgeFade[child.waypoint] : 1.0f);
        scene.setTransform(child.instance, assets::multiply(child.bouncer ? child.motion.matrix() : child.local, child.parent));
    }
    for (const Parts& entry : parts_) {
        const runtime::BuilditState& build = buildits.objects()[entry.buildit].build;
        const assets::Placement& parts = placed[entry.placedIndex];
        const runtime::NodeMatrix placement = assets::placementMatrix(parts);
        scene.setInstanceVisible(placedScene[entry.placedIndex], false);
        const bool visible = entry.placedIndex < shown.size() && shown[entry.placedIndex];
        for (std::size_t brick = 0; brick < entry.bricks.bricks().size() && brick < build.flight.size(); ++brick) {
            const std::ptrdiff_t instance = entry.nodeInstance.at(entry.bricks.bricks()[brick].node);
            if (instance < 0) continue;
            scene.setInstanceVisible(std::size_t(instance), visible);
            runtime::NodeMatrix node = entry.bricks.matrix(brick, build.flight[brick]);
            node[13] += buildits.objects()[entry.buildit].wobble.offset(brick);
            scene.setTransform(std::size_t(instance), assets::multiply(node, placement));
        }
        if (entry.finalIndex >= 0 && entry.rise.writes(build.propRise)) {
            assets::Placement risen = placed[std::size_t(entry.finalIndex)];
            risen.position[1] += build.propRise;
            scene.setTransform(placedScene[std::size_t(entry.finalIndex)], assets::placementMatrix(risen));
        }
    }
}

}
