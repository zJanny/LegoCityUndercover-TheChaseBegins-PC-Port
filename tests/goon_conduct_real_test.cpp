// The AI parameters of the characters of lvl_01 (src/runtime/goon_conduct.*) and the sight of the first fight of mission 1-4 on the level data.
// Values read by 20261103-large-C (2026-11-03) with the C++ level loader and aiConductParamsOf on lvl_01_cherrytreehills_main.blvl; the rules
// are those of docs/re/scripts-missions-1-4-spawners-conduct.md (objdump of game.arm.elf), not measured in the original. Needs the extracted RomFS; exits
// with 77 (skipped) when missing. AIRunSpeed and ReturnToSpawnPoint of lvl_01 added by 20261103-large-C on 2026-11-07 (same loader).
#include "assets/assets.hpp"
#include "runtime/fall_arrest_bounds.hpp"
#include "runtime/goon_conduct.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::optional<std::size_t> boundNamed(const assets::Level& level, const std::string& name) {
    for (std::size_t i = 0; i < level.bounds.size(); ++i)
        if (level.bounds[i].name == name) return i;
    return std::nullopt;
}

std::array<float, 3> placed(const assets::Level& level, const std::string& name) {
    const auto start = assets::objectStart(level, name);
    require(start.has_value(), "placed: " + name);
    return {start->position[0], start->position[1], start->position[2]};
}

}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: goon-conduct-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "goon-conduct-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
        const assets::FuseEntry* entry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
        require(entry != nullptr, "lvl_01 in the archive");
        const assets::Level level = assets::parseLevel(archive.read(*entry));

        std::map<std::string, runtime::AiConductParams> params;
        std::vector<std::string> others;
        std::size_t characters = 0, bounded = 0, friendly = 0, bad = 0, millAbout = 0, sleep = 0, patrol = 0;
        for (const assets::LevelObject& object : level.objects) {
            const assets::Level* cls = classOf(object.className);
            if (!cls || object.className.rfind("chars/", 0) != 0) continue;
            const runtime::AiConductParams p = runtime::aiConductParamsOf(object, *cls);
            params[object.instance] = p;
            ++characters;
            bounded += boundNamed(level, p.rangeBound).has_value();
            friendly += p.friendly;
            bad += p.badGuy;
            millAbout += p.millAbout;
            sleep += !p.idleReturn.sleepAnim.empty();
            patrol += p.patrolPath;
            if (p.runSpeed != runtime::kAiRunSpeedDefault * runtime::kAiRunSpeedScale || !p.returnToSpawnPoint) others.push_back(object.instance);
        }
        require(characters == 100 && bounded == 36 && friendly == 26 && bad == 52 && millAbout == 1 && sleep == 0 && patrol == 0,
                "lvl_01: 100 characters, 36 with an AIRangeBound that names a bound, 26 AIFriendly, 52 AIBadGuy, 1 AIMillAbout, no AISleepAnim or AIPatrolPath");

        require(others.size() == 4, "lvl_01: four characters set AIRunSpeed or ReturnToSpawnPoint 0");
        require(params.at("1-4_Booster_End").friendly && !params.at("1-4_Booster_End").returnToSpawnPoint &&
                    std::fabs(params.at("1-6_JimmyTheSnitch").runSpeed - 1.1f * runtime::kAiRunSpeedScale) < 1e-4f && params.at("1-6_JimmyTheSnitch").friendly,
                "the friendly characters with their own spawn-point values: 1-4_Booster_End (ReturnToSpawnPoint 0), 1-6_JimmyTheSnitch (AIRunSpeed 1.1)");

        const std::map<std::string, std::uint16_t> views{{"1-4_AI_Goon_01", 100}, {"1-4_AI_Goon_02", 100}, {"1-4_AI_Goon_03", 50}, {"1-4_AI_Goon_06", 50},
                                                         {"1-4_AI_Goon_07", 100}, {"1-4_AI_Goon_08", 100}, {"1-4_AI_Goon_09", 20}, {"1-4_AI_Goon_10", 25},
                                                         {"1-4_AI_Goon_11", 40},  {"1-4_AI_Goon_12", 40},  {"1-4_AI_Goon_13", 40}, {"1-4_AI_Goon_14", 100},
                                                         {"1-4_AI_Goon_15", 100}, {"1-4_AI_Goon_16", 100}, {"1-4_AI_Goon_17", 100}};
        for (const auto& [name, view] : views) {
            const runtime::AiConductParams& p = params.at(name);
            const bool roof = name == "1-4_AI_Goon_11" || name == "1-4_AI_Goon_12" || name == "1-4_AI_Goon_13";
            require(p.viewDistance == view && p.rangeBound == (roof ? "1-4_AIRange_03" : "1-4_AIRange_01") && p.badGuy && !p.friendly && !p.ignorePlayer &&
                        p.targetMode == 0 && p.fireDistance == 5.0f && p.meleeFireDistance == runtime::kAiMeleeFireDistanceDefault,
                    "the goon's AI parameters: " + name);
        }
        for (const char* name : {"1-4_Car_Salesman", "1-4_Cop_01", "1-4_Cop_02", "1-4_Booster_End"}) {
            const runtime::AiConductParams& p = params.at(name);
            require(p.friendly && p.ignorePlayer && p.targetMode == 1 && p.viewDistance == 3 && !p.badGuy && p.rangeBound == "0" && !boundNamed(level, p.rangeBound),
                    std::string("the friendly actors: ") + name);
        }
        const runtime::AiConductParams& booster = params.at("1-4_Booster_Strange");
        require(!booster.friendly && !booster.badGuy && booster.viewDistance == runtime::kAiViewDistanceDefault && booster.rangeBound.empty() &&
                    booster.fireDistance == runtime::kAiFireDistanceDefault,
                "Booster takes the class defaults and is on the side of the player");

        const auto range = boundNamed(level, "1-4_AIRange_01");
        require(range.has_value(), "the bound 1-4_AIRange_01");
        const assets::LevelBound& bound = level.bounds[*range];
        require(bound.shape == 3 && bound.node == 0xff && bound.enabled == 1 && std::fabs(bound.centre[0] - 560.654f) < 1e-3f && std::fabs(bound.centre[1] - 29.3624f) < 1e-3f &&
                    std::fabs(bound.centre[2] - 964.606f) < 1e-3f && std::fabs(bound.half[0] - 38.0018f) < 1e-3f && std::fabs(bound.half[1] - 7.1531f) < 1e-3f &&
                    std::fabs(bound.half[2] - 66.2311f) < 1e-3f,
                "1-4_AIRange_01: a box around the showroom");

        runtime::EngagedTable table;
        runtime::AiConduct conduct;
        std::vector<runtime::AiConductEntity> entities;
        for (const char* name : {"1-4_AI_Goon_03", "1-4_AI_Goon_06"}) {
            runtime::AiConductEntity goon;
            goon.name = name;
            goon.params = params.at(name);
            goon.rangeBound = boundNamed(level, goon.params.rangeBound);
            goon.position = placed(level, name);
            entities.push_back(goon);
        }
        runtime::AiConductEntity player;
        player.name = "player1";
        player.player = true;
        player.position = {600.0f, 27.5656f, 967.533f};
        entities.push_back(player);
        conduct.reset(table, entities);
        const runtime::AiConductBoundTest inside = [&](std::size_t index, const std::array<float, 3>& point) {
            return runtime::pointInFallArrestBound(level.bounds[index], level.bounds[index].enabled != 0, point);
        };
        conduct.update(inside);
        std::vector<runtime::AiConductEvent> events = conduct.takeEvents();
        require(events.size() == 2 && events[0].kind == runtime::AiConductEventKind::Engaged && events[0].slot == 0 && events[1].slot == 1,
                "with the player at x 600, beyond the bound's 598.656, both goons only take a slot");
        conduct.entities()[2].position = placed(level, "1-4_P1_GoTo");
        conduct.update(inside);
        events = conduct.takeEvents();
        require(events.size() == 2 && events[0].kind == runtime::AiConductEventKind::Sighted && events[0].character == "1-4_AI_Goon_03" &&
                    std::fabs(events[0].distance - 12.387f) < 1e-3f && events[1].character == "1-4_AI_Goon_06" && std::fabs(events[1].distance - 13.762f) < 1e-3f,
                "at 1-4_P1_GoTo (577.484 27.5656 967.533) both goons see the player, 12.387 and 13.762 away");
        require(conduct.entities()[0].aiState == runtime::kAiCombatState && conduct.entities()[1].aiState == runtime::kAiCombatState, "AI state 2");
    } catch (const std::exception& error) {
        std::cerr << "goon-conduct-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "goon-conduct-real-test: ok\n";
    return 0;
}
