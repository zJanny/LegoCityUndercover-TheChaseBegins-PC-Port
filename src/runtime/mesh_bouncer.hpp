#pragma once
// Class dummymeshbouncer (object type 228, descriptor 0x1c8008, create 0x260528, update 0x260728): a model that turns about the up axis and
// hops up and down. The waypoint classes carry one as their child "Badge" (docs/re/objective-hud.md). No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/prop_nodes.hpp"
#include <vector>

namespace chase::runtime {

constexpr float kBouncerRadiansPerRevPerMinute = 0.10471975803375244f;  // literal 0x260700
constexpr float kBouncerCyclesPerBouncePerMinute = 0.01666666753590107f;  // literal 0x260714
constexpr float kBouncerPeriod = 60.0f;                                    // literal 0x2607e4: the clock wraps here
constexpr float kBouncerTwoPi = 6.2831854820251465f;                       // literal 0x2607e8

struct MeshBouncerParams {
    float revsPerMin = 0;    // RevsPerMin (hash 7581f2b0), default 0
    float bouncePerMin = 0;  // BouncePerMin (hash ba47dfdf), default 0
    float bounceAmount = 0;  // BounceAmount (hash 40d85717), default 0
};

class MeshBouncer {
public:
    MeshBouncer(const NodeMatrix& base, const MeshBouncerParams& params);
    void update(float dt);
    // Node matrix relative to the parent: the base turned about y by revs * time, its translation raised by amount * sin(2 pi bounces * time).
    NodeMatrix matrix() const;
    float clock() const { return clock_; }

private:
    NodeMatrix base_;
    float spinRate_, bounceRate_, amount_;
    float clock_ = 0;
};

// A child object that a class file embeds (section +0x2c of levels/<class>.blvl), resolved against its own class.
struct PropChild {
    assets::Placement local;  // instance, className, mesh, position and orientation relative to the parent
    bool bouncer = false;
    MeshBouncerParams bouncerParams;
};
std::vector<PropChild> propChildren(const assets::Level& classFile, const assets::ClassLookup& classOf);

constexpr const char* kMeshBouncerClass = "dummymeshbouncer";

}
