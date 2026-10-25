// The zipline rope (src/runtime/zipline_rope.cpp) on a made-up line, no game data needed. Every expected value is a hand evaluation (numpy, float32) of
// the rules read in 0x1a9dc4 (fade), 0x1f9590 (strip) and the draw filter of docs/re/player-traversal.md on the anchors A (0, 10, 0) and B (20, 5, 0)
// that point at each other, with the stand-in view direction (0, 0, 1) of TRAV-S29.
#include "runtime/zipline_rope.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }

ZiplineAnchor anchor(const char* name, std::size_t order, float x, float y, float z, int target) {
    ZiplineAnchor result;
    result.instance = name;
    result.order = order;
    result.position[0] = x;
    result.position[1] = y;
    result.position[2] = z;
    result.target = target;
    return result;
}
}

int main() {
    try {
        const std::vector<ZiplineAnchor> anchors = {anchor("a", 0, 0, 10, 0, 1), anchor("b", 1, 20, 5, 0, 0)};
        ZiplineRope rope;
        rope.texture = "models/textures/rope.tga";
        const std::vector<ZiplineRope> ropes = {rope, rope};
        const float view[3] = {0, 0, 1};

        const float nearCamera[3] = {0, 10, 1.0f}, middleCamera[3] = {0, 10, 3.5f}, farCamera[3] = {0, 10, 6.0f};
        require(ziplineRopeAlpha(anchors[0], anchors[1], nearCamera) == -1, "alpha at distance 1");
        require(ziplineRopeAlpha(anchors[0], anchors[1], middleCamera) == 127, "alpha at distance 3.5 is (3.5 - 2) / 3 * 255 = 127");
        require(ziplineRopeAlpha(anchors[0], anchors[1], farCamera) == 255, "alpha at distance 6");
        const float lineCamera[3] = {10, 7.5f, 0};
        require(ziplineRopeAlpha(anchors[0], anchors[1], lineCamera) == -1, "a camera on the line is inside the fade distance");
        const float steepFar[3] = {6, 16, 0};
        const std::vector<ZiplineAnchor> vertical = {anchor("c", 0, 0, 10, 0, 1), anchor("d", 1, 0, 0, 0, 0)};
        require(ziplineRopeAlpha(vertical[0], vertical[1], steepFar) == 255, "alpha with a vertical line takes the other frame branch");

        const auto strip = ziplineRopeStrip(anchors[0].position, anchors[1].position, 0.03f, view, 255);
        const float expected[4][5] = {{-0.0072761f, 9.9708958f, 0, 1, 0}, {0.0072761f, 10.0291042f, 0, 0, 0}, {19.9927235f, 4.9708958f, 0, 1, 20}, {20.0072765f, 5.0291042f, 0, 0, 20}};
        for (int i = 0; i < 4; ++i) {
            const RopeVertex& v = strip[std::size_t(i)];
            require(near(v.position[0], expected[i][0], 1e-4f) && near(v.position[1], expected[i][1], 1e-4f) && near(v.position[2], expected[i][2], 1e-4f),
                    "vertex " + std::to_string(i) + " position");
            require(near(v.uv[0], expected[i][3], 1e-6f) && near(v.uv[1], expected[i][4], 1e-4f), "vertex " + std::to_string(i) + " uv");
            require(v.rgba[0] == 255 && v.rgba[1] == 255 && v.rgba[2] == 255 && v.rgba[3] == 255, "vertex colour");
        }

        // Provenance: gdb trap T46 (oracle save state under the zipline of mission 1-2, round 28 agent D, trap output T46): at 0x1f9590 r0 = 892.20288 64.4198 632.59888
        // (start), r1 = 868.09351 69.9924 630.97711 (end); at 0x1f95bc the row at 0x529fec = (0.96139693, -0.1675961, -0.21823755), the camera's view
        // direction (its horizontal part matches the camera yaw 1.794 of the player report). The side vector is the formula of 0x1f95bc..0x1f95e4;
        // the expected vertices come from it (numpy), not from a measured vertex buffer.
        {
            const float measuredStart[3] = {892.20288f, 64.4197998f, 632.598877f}, measuredEnd[3] = {868.093506f, 69.9924011f, 630.977112f};
            const float measuredRow[3] = {0.961396933f, -0.167596102f, -0.218237549f};
            const auto measured = ziplineRopeStrip(measuredStart, measuredEnd, 0.03f, measuredRow, 255);
            const float expectedStart[2][3] = {{892.1965975f, 64.39099708f, 632.59331615f}, {892.20916422f, 64.44860253f, 632.60443775f}};
            for (int i = 0; i < 2; ++i)
                for (int axis = 0; axis < 3; ++axis) require(near(measured[std::size_t(i)].position[axis], expectedStart[i][axis], 2e-3f), "measured strip vertex " + std::to_string(i));
        }

        // Provenance: the state record 0x252254 builds (read, round 28): 0x60 bytes of zero, then byte 0xa = 9 (one texture), 0xb = 8 (depth write) with bit 6
        // set, 0xc = 0x3e (cull none), 0xe = 0xcb, colours 0x10 / 0x14 = 0xffffffff, byte 6 = 127; 0x297b7c sets byte 5 = 1, bit 6 of 0xb cleared and the mode
        // bytes 8 / 9 to (1, 0) with bit 3 set (alpha 255) or (4, 5) with bit 3 cleared (other alpha). Fed through the port's reading of the record (0x128dc8,
        // 0x1292fc) it gives the render state the rope uses.
        for (const bool faded : {false, true}) {
            chase::assets::ModelPart part;
            part.material.assign(0x60, 0);
            auto& m = part.material;
            m[0xa] = 9;
            m[0xb] = faded ? 0 : 8;
            m[0xc] = 0x3e;
            m[0xe] = 0xcb;
            for (int i = 0x10; i < 0x18; ++i) m[std::size_t(i)] = 0xff;
            m[5] = 1;
            m[6] = 127;
            m[8] = faded ? 4 : 1;
            m[9] = faded ? 5 : 0;
            part.textureSlots = {0, 0, 0, 0, 0, 0, 5, 0};
            const chase::assets::MaterialState state = chase::assets::materialState(part);
            require(state.supported && !state.alphaTest && state.depthTest && state.depthFunction == 5 && state.cullMode == 0 && state.depthWrite == !faded, "rope depth, cull, alpha test");
            require(state.blendEquation == 0 && state.blendSource == (faded ? 6u : 1u) && state.blendDestination == (faded ? 7u : 0u), "rope blend factors");
            require(state.fogged && state.unitSlot[0] == 0 && state.unitSlot[1] == -1, "rope fog and texture unit");
        }

        const auto all = [](const ZiplineAnchor&) { return true; };
        const auto draws = ziplineRopeDraws(anchors, ropes, farCamera, view, all);
        require(draws.size() == 2, "two draws per textured line (gate 0x226644, docs/re/player-traversal.md): " + std::to_string(draws.size()));
        require(near(draws[0].strip[0].position[0], 0.0f, 0.04f) && near(draws[1].strip[0].position[0], 20.0f, 0.04f), "each anchor draws from its own node");
        require(draws[0].texture == rope.texture && draws[0].alpha == 255, "draw texture and alpha");

        const auto bDisabled = [](const ZiplineAnchor& a) { return a.instance != "b"; };
        const auto backup = ziplineRopeDraws(anchors, ropes, farCamera, view, bDisabled);
        require(backup.size() == 1 && near(backup[0].strip[0].position[0], 0.0f, 0.04f), "only the enabled anchor draws");
        const auto bothDisabled = [](const ZiplineAnchor&) { return false; };
        require(ziplineRopeDraws(anchors, ropes, farCamera, view, bothDisabled).empty(), "no draw when both are disabled");

        std::vector<ZiplineRope> fading = ropes;
        fading[0].fadeNearCamera = fading[1].fadeNearCamera = true;
        const auto faded = ziplineRopeDraws(anchors, fading, middleCamera, view, all);
        require(faded.size() == 2 && faded[0].alpha != 255 && faded[1].alpha != 255, "a fading rope near the camera is not opaque and blends once per anchor");
        require(ziplineRopeDraws(anchors, fading, nearCamera, view, all).empty(), "a fading rope inside the fade distance is not drawn");

        std::vector<ZiplineRope> bare = ropes;
        bare[1].texture.clear();
        require(ziplineRopeDraws(anchors, bare, farCamera, view, all).empty(), "the higher id anchor without RopeTexture is live (byte +0x13 = 2) and the lower id anchor leaves the line to it");
        require(ziplineRopeDraws(anchors, bare, farCamera, view, bDisabled).size() == 1, "a disabled target is not live, so the lower id anchor draws");
        bare[1].texture = rope.texture;
        bare[0].texture.clear();
        const auto lowerBare = ziplineRopeDraws(anchors, bare, farCamera, view, all);
        require(lowerBare.size() == 1 && near(lowerBare[0].strip[0].position[0], 20.0f, 0.04f), "the lower id anchor without RopeTexture draws nothing, the other draws");
        std::cout << "zipline-rope-test: ok\n";
    } catch (const std::exception& error) {
        std::cerr << "zipline-rope-test: " << error.what() << '\n';
        return 1;
    }
}
