#include "runtime/mesh_bouncer.hpp"
#include "runtime/level_start.hpp"

#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {

float floatParam(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param || param->type != 0) return 0.0f;
    float value;
    std::memcpy(&value, &param->value, 4);
    return value;
}

}

MeshBouncer::MeshBouncer(const NodeMatrix& base, const MeshBouncerParams& params)
    : base_(base),
      spinRate_(params.revsPerMin * kBouncerRadiansPerRevPerMinute),
      bounceRate_(params.bouncePerMin * kBouncerCyclesPerBouncePerMinute),
      amount_(params.bounceAmount) {}

void MeshBouncer::update(float dt) {
    clock_ += dt;
    if (kBouncerPeriod <= clock_) clock_ -= kBouncerPeriod;
}

NodeMatrix MeshBouncer::matrix() const {
    NodeMatrix m = base_;
    const float angle = spinRate_ * clock_;
    const float s = std::sin(angle), c = std::cos(angle);
    for (int column = 0; column < 3; ++column) {
        const float row0 = m[std::size_t(column)], row2 = m[std::size_t(8 + column)];
        m[std::size_t(column)] = row0 * c - row2 * s;
        m[std::size_t(8 + column)] = row2 * c + row0 * s;
    }
    m[13] = base_[13] + std::sin(bounceRate_ * clock_ * kBouncerTwoPi) * amount_;
    return m;
}

std::vector<PropChild> propChildren(const assets::Level& classFile, const assets::ClassLookup& classOf) {
    std::vector<PropChild> out;
    for (const assets::Placement& placement : assets::objectPlacements(classFile, classOf)) {
        PropChild child;
        child.local = placement;
        child.bouncer = placement.className == kMeshBouncerClass;
        if (child.bouncer) {
            const assets::LevelObject& object = classFile.objects[placement.objectIndex];
            const assets::Level* cls = classOf(object.className);
            child.bouncerParams.revsPerMin = floatParam(object, *cls, "RevsPerMin");
            child.bouncerParams.bouncePerMin = floatParam(object, *cls, "BouncePerMin");
            child.bouncerParams.bounceAmount = floatParam(object, *cls, "BounceAmount");
        }
        out.push_back(std::move(child));
    }
    return out;
}

}
