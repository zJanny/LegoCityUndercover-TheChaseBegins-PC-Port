#include "game_level.hpp"

#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

namespace {

class SessionParticleWorld : public runtime::ParticleObjectWorld {
public:
    SessionParticleWorld(runtime::ParticleSystem& system, runtime::GameRandom& random) : system_(system), random_(random) {}

    std::uint32_t start(std::int32_t definition, const std::array<float, 3>& position) override {
        if (definition < 0) return 0;
        return system_.start(unsigned(definition), {position, std::nullopt, 0});
    }
    void stop(std::uint32_t emitter, float fadeSeconds) override { system_.stop(emitter, fadeSeconds); }
    float random() override { return float(random_.next() & 0xffffu) * (1.0f / 65536.0f); }
    bool running(std::uint32_t emitter) const override { return system_.find(emitter) != nullptr; }

private:
    runtime::ParticleSystem& system_;
    runtime::GameRandom& random_;
};

}

void LevelSession::loadParticleObjects() {
    SessionParticleWorld world(particles.system(), particles.random());
    const chase::assets::ParticleDefs& defs = particles.system().defs();
    particleObjects.load(level, classOf, [&defs](std::int32_t index) {
        runtime::ParticleObjectDefinition definition;
        if (index >= 0 && std::size_t(index) < defs.defs.size()) {
            definition.duration = defs.defs[std::size_t(index)].duration;
            definition.lifetimeMax = defs.defs[std::size_t(index)].lifetimeMax;
        }
        return definition;
    }, world);
    particleObjectsStarted.clear();
    fallArrestBoundList = runtime::fallArrestBounds(level);
    std::cout << "particle objects: " << particleObjects.objects().size() << ", fall arrest bounds: " << fallArrestBoundList.size() << '\n';
}

void LevelSession::deliverParticleObjectMessage(const runtime::ScriptObjectMessage& sent) {
    if (!particleObjects.find(sent.instance)) return;
    SessionParticleWorld world(particles.system(), particles.random());
    particleObjects.message(sent.instance, sent.message, world);
}

// STAND-IN SCRIPT-S325: the update runs with the level's other class updates after the script runner, an order the port chose. The engaged table
// 0x58deb0 is not modelled, so the FallArrest update 0x307980 has no characters to test (SCRIPT-425).
void LevelSession::updateParticleObjects(float dt) {
    SessionParticleWorld world(particles.system(), particles.random());
    particleObjects.update(dt, world);
    for (const runtime::ParticleObjectEvent& event : particleObjects.takeEvents()) {
        switch (event.kind) {
        case runtime::ParticleObjectEventKind::Started:
            particleObjectsStarted.insert(event.instance);
            std::cout << "particle object " << event.instance << ": effect " << event.definition << " started at update " << updatesForLog << '\n';
            break;
        case runtime::ParticleObjectEventKind::Refused:
            std::cout << "particle object " << event.instance << ": effect " << event.definition << " refused at update " << updatesForLog << '\n';
            break;
        case runtime::ParticleObjectEventKind::Stopped:
            std::cout << "particle object " << event.instance << ": effect " << event.definition << " stopped over " << event.fadeSeconds << " s at update "
                      << updatesForLog << '\n';
            break;
        case runtime::ParticleObjectEventKind::Disabled:
            if (particleObjectsStarted.count(event.instance))
                std::cout << "particle object " << event.instance << ": disabled at update " << updatesForLog << '\n';
            break;
        }
    }
}

}
