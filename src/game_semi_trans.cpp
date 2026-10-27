#include "game_semi_trans.hpp"

#include "assets/object_collision.hpp"

#include <algorithm>

namespace game_semi_trans {

namespace {

std::string modelStem(const std::string& path) {
    const std::size_t slash = path.find_last_of("/\\");
    std::string name = chase::render::lower(slash == std::string::npos ? path : path.substr(slash + 1));
    const std::size_t dot = name.find('.');
    return dot == std::string::npos ? name : name.substr(0, dot);
}

}

runtime::NearCameraView followView(const float eye[3], float yaw, float pitch, const float player[3]) {
    runtime::NearCameraView view;
    float right[3];
    chase::render::cameraAxes(yaw, pitch, view.forward.data(), right);
    std::copy_n(eye, 3, view.eye.begin());
    std::copy_n(player, 3, view.target.begin());
    return view;
}

void NearCameraFades::update(chase::assets::FuseArchive& archive, const chase::render::Scene& scene, const std::string& levelName,
                             const std::vector<runtime::SemiTransNearCameraEntry>& entries, const runtime::NearCameraView& view) {
    for (; seenEntries_ < entries.size(); ++seenEntries_) {
        const std::string model = modelStem(entries[seenEntries_].model);
        const std::string atlas = model + "_atlas_" + chase::render::lower(levelName);
        for (std::size_t instance = 0; instance < scene.instances(); ++instance) {
            const std::string stem = modelStem(scene.instanceModel(instance));
            if (stem != model && stem != atlas) continue;
            if (std::any_of(tracked_.begin(), tracked_.end(), [&](const Tracked& known) { return known.instance == instance; })) continue;
            Tracked entry{instance, {}, {}};
            if (!chase::assets::modelBounds(archive, scene.instanceModel(instance), entry.centre.data(), entry.half.data())) continue;
            tracked_.push_back(entry);
        }
    }
    const runtime::Vector3 direction = runtime::nearCameraDirection(view);
    for (Tracked& entry : tracked_) {
        const chase::render::Mat4 transform = scene.instanceTransform(entry.instance);
        const runtime::Vector3 centre{entry.centre[0] + transform[12], entry.centre[1] + transform[13], entry.centre[2] + transform[14]};
        entry.alpha = runtime::semiTransStep(entry.alpha, runtime::nearCameraTarget(view, direction, centre, entry.half));
    }
}

void NearCameraFades::apply(chase::render::Scene& scene) {
    for (Tracked& entry : tracked_) {
        float base = scene.instanceFade(entry.instance);
        if (base == entry.written) base = entry.base;
        entry.base = base;
        entry.written = std::min(base, float(runtime::semiTransNodeAlpha(entry.alpha)) / 255.0f);
        scene.setInstanceFade(entry.instance, entry.written);
    }
}

}
