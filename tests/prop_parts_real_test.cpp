// Moving parts of placed props from the extracted RomFS: the bricks of 1-1_BI_DonutVanParts (model nodes Bit1..Bit6 and the first and last frame of
// models/prop_donut_truck_cabin_bi/prop_donut_truck_cabin_bi_buildit.bfnanm) and the Badge child that the waypoint classes embed
// (levels/props/map_waypoint.blvl, map_vehiclewaypoint.blvl). Reference values: dump of the model block +0x20, of the animation's object blocks
// decoded with objectAnimationMatrix, and of the class files (docs/re/buildit.md, docs/re/objective-hud.md). Exits with 77 without data.
#include "assets/assets.hpp"
#include "runtime/buildit_bricks.hpp"
#include "runtime/mesh_bouncer.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: prop-parts-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "prop-parts-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const std::string mesh = "Prop_Donut_Truck_Cabin_BI/Prop_Donut_Truck_Cabin_BI_atlas_lvl_01_cherrytreehills_main";
        require(runtime::BuilditBricks::animationPath(mesh) == "models/prop_donut_truck_cabin_bi/prop_donut_truck_cabin_bi_buildit.bfnanm", "animation path");
        require(archive.find(runtime::BuilditBricks::animationPath(mesh)) != nullptr, "the animation is in the archive");
        const runtime::BuilditBricks bricks = runtime::BuilditBricks::load(archive, mesh);
        require(bricks.bricks().size() == 6, "six bricks");
        // Frame 0 of the object blocks Bit1 and Bit6 (anim dump: translation row of the decoded matrix); the model's own node matrices agree within the
        // animation's 0.0025 quantisation.
        const runtime::NodePose& first = bricks.bricks()[0].start;
        require(near(first.position[0], -0.510625f, 1e-4f) && near(first.position[1], 1.045f, 1e-4f) && near(first.position[2], -1.54f, 1e-4f), "Bit1 scattered position");
        const runtime::NodePose& last = bricks.bricks()[5].start;
        require(near(last.position[0], -1.3425f, 1e-4f) && near(last.position[1], 3.16875f, 1e-4f) && near(last.position[2], -5.13438f, 1e-4f), "Bit6 scattered position");
        for (const runtime::BuilditBrick& brick : bricks.bricks()) {
            require(near(brick.end.position[0], 0, 1e-6f) && near(brick.end.position[1], 0, 1e-6f) && near(brick.end.position[2], 0, 1e-6f), "built pose has no offset");
            require(near(brick.end.rotation[3], 1, 1e-6f), "built pose has no rotation");
            const runtime::NodeMatrix built = bricks.matrix(std::size_t(&brick - &bricks.bricks()[0]), 1.0f);
            require(near(built[0], 1, 1e-5f) && near(built[5], 1, 1e-5f) && near(built[10], 1, 1e-5f) && near(built[13], 0, 1e-5f), "t = 1 is the identity");
        }
        const assets::FuseEntry* modelEntry = archive.find(assets::meshModelPath(mesh));
        require(modelEntry != nullptr, "parts model");
        const assets::Model model = assets::parseModel(archive.read(*modelEntry));
        for (std::size_t i = 0; i < 6; ++i) {
            const runtime::NodeMatrix scatteredPose = bricks.matrix(i, 0.0f);
            for (std::size_t k = 0; k < 16; ++k)
                require(near(scatteredPose[k], model.nodeMatrices[i][k], 0.012f), "brick " + std::to_string(i) + " scattered pose equals the model's node matrix element " + std::to_string(k));
        }
        const runtime::NodeMatrix scattered = bricks.matrix(0, 0.0f);
        require(near(scattered[12], first.position[0], 1e-5f) && near(scattered[1], -1.0f, 2e-3f) && near(scattered[4], 1.0f, 2e-3f), "t = 0 is the scattered pose of Bit1 (a quarter turn about z)");

        for (const char* className : {"props/map_waypoint", "props/map_vehiclewaypoint", "props/map_getintovehiclewaypoint"}) {
            const assets::FuseEntry* entry = archive.find(std::string("levels/") + className + ".blvl");
            require(entry != nullptr, std::string("class file ") + className);
            const assets::Level classFile = assets::parseLevel(archive.read(*entry));
            const auto children = runtime::propChildren(classFile, assets::archiveClassLookup(archive));
            require(children.size() == 1 && children[0].local.instance == "Badge" && children[0].bouncer, std::string(className) + " has one bouncer child Badge");
            require(children[0].local.mesh == "Prop_Waypoint_LCPDBadge", "Badge mesh");
            const bool small = std::string(className) == "props/map_waypoint";
            require(near(children[0].bouncerParams.revsPerMin, 15.0f, 1e-6f), "RevsPerMin 15");
            require(near(children[0].bouncerParams.bouncePerMin, small ? 50.0f : 20.0f, 1e-6f), "BouncePerMin");
            require(near(children[0].bouncerParams.bounceAmount, small ? 0.08f : 0.25f, 1e-6f), "BounceAmount");
            require(near(children[0].local.position[1], small ? 0.3965f : 2.6375f, 1e-3f), "Badge position");
        }
    } catch (const std::exception& error) {
        std::cerr << "prop-parts-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "prop-parts-real-test: ok\n";
    return 0;
}
