// The zipline rope (src/runtime/zipline_rope.cpp) on a made-up line, no game data needed. Every expected value is a hand evaluation (numpy, float32) of
// the rules read in 0x1a9dc4 (fade), 0x1f9590 (strip) and the draw filter of docs/re/player-traversal.md on the anchors A (0, 10, 0) and B (20, 5, 0)
// that point at each other, with the stand-in view direction (0, 0, 1) of TRAV-S27.
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

        const auto all = [](const ZiplineAnchor&) { return true; };
        const auto draws = ziplineRopeDraws(anchors, ropes, farCamera, view, all);
        require(draws.size() == 1, "one draw per line: " + std::to_string(draws.size()));
        require(near(draws[0].strip[0].position[0], 20.0f, 0.04f), "the higher entity id draws, from its own node");
        require(draws[0].texture == rope.texture && draws[0].alpha == 255, "draw texture and alpha");

        const auto bDisabled = [](const ZiplineAnchor& a) { return a.instance != "b"; };
        const auto backup = ziplineRopeDraws(anchors, ropes, farCamera, view, bDisabled);
        require(backup.size() == 1 && near(backup[0].strip[0].position[0], 0.0f, 0.04f), "the lower id anchor draws when its target is disabled");
        const auto bothDisabled = [](const ZiplineAnchor&) { return false; };
        require(ziplineRopeDraws(anchors, ropes, farCamera, view, bothDisabled).empty(), "no draw when both are disabled");

        std::vector<ZiplineRope> fading = ropes;
        fading[1].fadeNearCamera = true;
        const auto faded = ziplineRopeDraws(anchors, fading, middleCamera, view, all);
        require(faded.empty() || faded[0].alpha != 255, "a fading rope near the camera is not opaque");
        require(ziplineRopeDraws(anchors, fading, nearCamera, view, all).empty(), "a fading rope inside the fade distance is not drawn");

        std::vector<ZiplineRope> bare = ropes;
        bare[1].texture.clear();
        require(ziplineRopeDraws(anchors, bare, farCamera, view, all).empty(), "an anchor without RopeTexture draws nothing");
        std::cout << "zipline-rope-test: ok\n";
    } catch (const std::exception& error) {
        std::cerr << "zipline-rope-test: " << error.what() << '\n';
        return 1;
    }
}
