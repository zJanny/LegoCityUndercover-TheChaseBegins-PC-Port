// The ropes of the zipline anchors of lvl_01_cherrytreehills (src/runtime/zipline_rope.cpp): RopeTexture, RenderWidth and FadeNearCamera of the class
// props/parkour_zipline read from the level data (probe of round 27 agent D, 2026-10-19), and the strip of 1-8_Zip_Line05 -> 1-8_Zip_Line04 with the
// stand-in view direction (0, 0, 1) (TRAV-S29), expected values evaluated with numpy float32 from the node positions in the level data. Needs the
// extracted RomFS; exits with 77 without it.
#include "assets/assets.hpp"
#include "runtime/fog.hpp"
#include "runtime/zipline_rope.hpp"
#include <array>
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
        std::cerr << "usage: zipline-rope-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "zipline-rope-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        using namespace chase;
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
        require(entry != nullptr, "level not in archive");
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const auto classOf = assets::archiveClassLookup(archive);
        const std::vector<runtime::ZiplineAnchor> anchors = runtime::loadZiplineAnchors(level, classOf);
        const std::vector<runtime::ZiplineRope> ropes = runtime::loadZiplineRopes(level, classOf, anchors);
        require(ropes.size() == anchors.size() && anchors.size() == 24, "24 ropes");
        int line05 = -1, line04 = -1;
        for (std::size_t i = 0; i < anchors.size(); ++i) {
            require(ropes[i].texture == "models/textures/prop_generic_ropealphat_a3i5.tga", "RopeTexture of " + anchors[i].instance + ": " + ropes[i].texture);
            require(near(ropes[i].width, runtime::kRopeRenderWidthDefault, 1e-7f) && !ropes[i].fadeNearCamera, "RenderWidth and FadeNearCamera of " + anchors[i].instance);
            if (anchors[i].instance == "1-8_zip_line05") line05 = int(i);
            if (anchors[i].instance == "1-8_zip_line04") line04 = int(i);
        }
        require(line05 >= 0 && line04 >= 0, "1-8_Zip_Line04 and 05 are anchors");
        require(anchors[std::size_t(line05)].target == line04 && anchors[std::size_t(line04)].target == line05, "the two anchors point at each other");

        const float camera[3] = {870.0f, 66.0f, 631.0f}, view[3] = {0, 0, 1};
        const auto all = [](const runtime::ZiplineAnchor&) { return true; };
        const auto draws = runtime::ziplineRopeDraws(anchors, ropes, camera, view, all);
        require(draws.size() == 24, "12 lines, two draws each (every anchor has RopeTexture and no update handler, gate 0x226644; the save state under the zipline of mission 1-2 trap T46 showed two 0x1f9590 calls per update for the Line04 / Line05 line): " + std::to_string(draws.size()));
        const runtime::RopeDraw* line = nullptr;
        for (const runtime::RopeDraw& draw : draws)
            if (near(draw.strip[0].position[0], 868.09f, 0.05f) && near(draw.strip[0].position[1], 70.0f, 0.1f)) line = &draw;
        require(line != nullptr, "the line of 1-8_Zip_Line05 is drawn from its own node");
        const float expected[4][4] = {{868.0867f, 69.9632f, 630.9771f, 0}, {868.1003f, 70.0216f, 630.9771f, 0}, {892.1961f, 64.3906f, 632.5989f, 24.1094f},
                                      {892.2097f, 64.449f, 632.5989f, 24.1094f}};
        for (int i = 0; i < 4; ++i) {
            const runtime::RopeVertex& v = line->strip[std::size_t(i)];
            require(near(v.position[0], expected[i][0], 2e-3f) && near(v.position[1], expected[i][1], 2e-3f) && near(v.position[2], expected[i][2], 2e-3f),
                    "vertex " + std::to_string(i));
            require(near(v.uv[1], expected[i][3], 2e-3f), "vertex v " + std::to_string(i));
        }
        const std::vector<assets::FogObject> fogObjects = assets::fogObjects(level, classOf);
        const runtime::FogSetting fog = runtime::levelFog(fogObjects, 7);
        require(fog.enabled && fog.start == 220.0f && fog.end == 260.0f, "rope fog range 220 260: the scene's fog (LevelSession::applyFog), pinned in fog-real-data from the oracle 0x376160 / 0x376164; the rope state record 0x252254 / 0x297b7c clears byte 0xb bit 6, so fog applies");
        require(fog.colour == std::array<std::uint8_t, 3>({183, 199, 221}), "rope fog colour (183, 199, 221): oracle 0x376168, pinned in fog-real-data");
        std::cout << "zipline-rope-real-test: ok\n";
    } catch (const std::exception& error) {
        std::cerr << "zipline-rope-real-test: " << error.what() << '\n';
        return 1;
    }
}
