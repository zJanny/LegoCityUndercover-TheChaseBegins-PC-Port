// The particle objects and FallArrest bounds of mission 1-4 in lvl_01_cherrytreehills_main.blvl: LadderKick (effect 148, fired by the TriggerObject
// of 1-4_SO_Phase2 after the goon's ladder kick) and LadderLand (effect 11, 1-4_AI_Goon_10_Run_Off_Script), the six bounds FallArrestBound01..07
// of the roof, and a trigger of LadderKick through ParticleObjects with the real definitions. Values recorded by 20261102-mission14b-C (2026-11-02)
// with the C++ level loader and parseParticleDefs (scratch dump of the level objects and bounds, not kept). Needs the extracted RomFS; exits with
// 77 (skipped) when missing.
#include "assets/assets.hpp"
#include "runtime/fall_arrest_bounds.hpp"
#include "runtime/particle_object.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) < 0.001f; }

class RecordingWorld : public runtime::ParticleObjectWorld {
public:
    std::uint32_t start(std::int32_t definition, const std::array<float, 3>& position) override {
        started.push_back({definition, position});
        return std::uint32_t(started.size());
    }
    void stop(std::uint32_t, float) override {}
    float random() override { return 0.0f; }
    bool running(std::uint32_t) const override { return true; }
    std::vector<std::pair<std::int32_t, std::array<float, 3>>> started;
};

}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: particle-object-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "particle-object-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
        require(entry != nullptr, "lvl_01 not in archive");
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const assets::FuseEntry* defsEntry = archive.find("particles/particledefs.bprt");
        require(defsEntry != nullptr, "particledefs.bprt not in archive");
        const assets::ParticleDefs defs = assets::parseParticleDefs(archive.read(*defsEntry));
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);

        RecordingWorld world;
        runtime::ParticleObjects objects;
        objects.load(level, classOf, [&defs](std::int32_t index) {
            runtime::ParticleObjectDefinition definition;
            if (index >= 0 && std::size_t(index) < defs.defs.size()) definition = {defs.defs[std::size_t(index)].duration, defs.defs[std::size_t(index)].lifetimeMax};
            return definition;
        }, world);
        for (const runtime::ParticleObject& object : objects.objects())
            std::cout << object.instance << ": effect " << object.params.definition << " at " << object.position[0] << ' ' << object.position[1] << ' '
                      << object.position[2] << ", retrigger " << object.params.reTriggerTime << ", duration " << object.definition.duration << ", lifetime "
                      << object.definition.lifetimeMax << ", endless " << object.endless << '\n';
        require(objects.objects().size() == 11, "11 objects of kind 1 in lvl_01");
        struct Expected {
            const char* name;
            std::int32_t definition;
            float x, y, z, duration, lifetime;
        };
        for (const Expected& expected : {Expected{"ladderkick", 148, 543.035f, 36.5175f, 988.892f, 0.2f, 0.2f},
                                         Expected{"ladderland", 11, 541.674f, 28.241f, 988.84f, 0.51f, 0.5f},
                                         Expected{"boosterland", 11, 546.443f, 28.2593f, 979.68f, 0.51f, 0.5f}}) {
            const runtime::ParticleObject* object = objects.find(expected.name);
            require(object != nullptr, std::string("particle object missing: ") + expected.name);
            require(object->params.definition == expected.definition && object->params.reTriggerTime == 0.0f && object->params.timeRandomiser == 0.0f &&
                        !object->params.instance && !object->params.justMoveSpawnPoint && object->params.sound == 0 && object->params.damage == 0.0f,
                    std::string(expected.name) + ": ParticleDef and the class defaults of particles.blvl");
            require(near(object->position[0], expected.x) && near(object->position[1], expected.y) && near(object->position[2], expected.z),
                    std::string(expected.name) + ": initialposition");
            require(near(object->definition.duration, expected.duration) && near(object->definition.lifetimeMax, expected.lifetime) && !object->endless,
                    std::string(expected.name) + ": a one-shot effect (+0x38 below 98)");
            require(object->timer == object->definition.duration, std::string(expected.name) + ": the timer starts at the duration");
        }
        objects.message("ladderkick", runtime::kParticleObjectMessageTrigger, world);
        require(world.started.size() == 1 && world.started[0].first == 148 && near(world.started[0].second[0], 543.035f) &&
                    near(world.started[0].second[1], 36.5175f) && near(world.started[0].second[2], 988.892f),
                "TriggerObject LadderKick 1 starts effect 148 at the object");

        const std::vector<std::size_t> bounds = runtime::fallArrestBounds(level);
        std::vector<std::string> names;
        for (const std::size_t bound : bounds) names.push_back(level.bounds[bound].name);
        require(names == std::vector<std::string>({"FallArrestBound01", "FallArrestBound02", "FallArrestBound03", "FallArrestBound04", "FallArrestBound05",
                                                   "FallArrestBound07"}),
                "the FallArrest bounds of lvl_01 (1-4_FallArrestBoundsON / OFF do not begin with the prefix)");
        std::vector<std::uint8_t> enabled(level.bounds.size(), 1);
        std::vector<runtime::FallArrestCharacter> engaged(1);
        engaged[0].name = "1-4_ai_goon_09";
        engaged[0].onGround = false;
        engaged[0].position = {505.513f, 22.06f, 961.079f};
        const std::vector<runtime::FallArrestHit> hits = runtime::fallArrestUpdate(level, bounds, enabled, engaged);
        require(hits.size() == 1 && level.bounds[hits[0].bound].name == "FallArrestBound05" && engaged[0].arrestMark == 2,
                "a goon in the air at the centre of FallArrestBound05 (505.513 22.06 961.079, level data) is marked");
    } catch (const std::exception& error) {
        std::cerr << "particle-object-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "particle-object-real-test: ok\n";
    return 0;
}
