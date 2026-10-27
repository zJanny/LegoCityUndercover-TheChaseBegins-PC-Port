// Shadows of the original (src/runtime/shadow.cpp, docs/re/rendering-shadows.md). Core: the shadow constant, the focus, the shadow
// camera and its projection against the values the oracle read outside the police station after mission 1-1 (RPC request
// 20261027-graphics-B-1 of round 20261027-graphics, run by 20261027-graphics-servant on 2026-10-27, recorded from a save state, OpenGL),
// and the light and flag rules transcribed from 0x15dd50, 0x22dedc and 0x1c22d0. With two arguments (FIB NAMES) the lvl_01 shadow light
// and the class flags of the shipped files are checked too; exits with 77 (skipped) when the data is missing.
#include "assets/assets.hpp"
#include "runtime/shadow.hpp"
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace chase::runtime;
namespace assets = chase::assets;

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) < tolerance; }

float rawFloat(std::uint32_t word) {
    float value;
    std::memcpy(&value, &word, 4);
    return value;
}

// lvl_01 Directional01 InitialOrientation, raw words of the level file (33335d42 66662242 0000c342).
assets::Placement directional01() {
    assets::Placement placement;
    placement.orientation[0] = rawFloat(0x425d3333);
    placement.orientation[1] = rawFloat(0x42226666);
    placement.orientation[2] = rawFloat(0x42c30000);
    return placement;
}

// The camera of chase-game at the oracle pose (player (900.549, 16.567, 1110.280) facing -x, `--start "900.549 16.567 1110.28" --facing
// 270`, 60 updates; printed by 20261027-graphics-B on 2026-10-27): rows right, up, forward and the eye.
ShadowMatrix portCamera() {
    return {0.004370f, 0.0f, 0.999990f, 0, -0.167592f, 0.985856f, 0.000732f, 0, -0.985847f, -0.167594f, 0.004308f, 0,
            910.549072f, 20.466995f, 1110.236328f, 1};
}

void constant() {
    const auto value = shadowConstant({0, 0, 0}, 0.6f);
    require(value[0] == 0x99 && value[1] == 0x99 && value[2] == 0x99, "0x377984 = 0x00999999 for black at opacity 0.6 (oracle B-1)");
    const auto shadowed = shadowFactor(value, 0.0f), lit = shadowFactor(value, 1.0f);
    require(near(shadowed[0], 0.4f, 1e-6f) && near(lit[1], 1.0f, 1e-6f), "the stages leave 1 - 153 / 255 in shadow and 1 when lit");
    const auto grey = shadowConstant({100, 50, 255}, 0.5f);
    require(grey[0] == 78 && grey[1] == 103 && grey[2] == 0, "trunc((255 - colour) * opacity + 0.5) per channel");
}

void focus() {
    const auto point = shadowFocus(portCamera(), kShadowRayUp, kShadowRayForward, 16.567f, kShadowRadiusDefault);
    require(point.has_value(), "the camera ray meets the player's height");
    require(near((*point)[0], rawFloat(0x445dae07), 0.02f) && near((*point)[1], rawFloat(0x4193a60c), 0.001f) &&
                near((*point)[2], rawFloat(0x448acb50), 0.02f),
            "focus equals the oracle's 0x530844 (886.719, 18.456, 1110.354)");
    require(!shadowFocus(portCamera(), kShadowRayUp, kShadowRayForward, 40.0f, kShadowRadiusDefault), "no focus a radius above the camera");
    ShadowMatrix level = portCamera();
    level[9] = 0.0f;
    require(!shadowFocus(level, kShadowRayUp, kShadowRayForward, 16.567f, kShadowRadiusDefault), "no focus when the ray does not point down");
}

void camera() {
    const ShadowMatrix light = assets::placementMatrix(directional01());
    const std::array<float, 3> centre = {rawFloat(0x445dae07), rawFloat(0x4193a60c), rawFloat(0x448acb50)};
    const ShadowCamera shadow = shadowCamera(light, centre, portCamera(), kShadowRadiusDefault);
    const std::uint32_t rows[9] = {0xbdcaf782, 0x3efd38c2, 0x3f5d0c2c, 0xbf40b5e7, 0xbf1ad17b, 0x3e851d9b, 0x3f269924, 0xbf1fcd90, 0x3edd4e3c};
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            require(near(shadow.world[std::size_t(r * 4 + c)], rawFloat(rows[r * 3 + c]), 2e-3f), "shadow camera axes equal the oracle's 0x52a434");
    require(near(shadow.world[13], rawFloat(0x42492ad5), 0.001f) && near(shadow.world[14], rawFloat(0x448809e7), 0.001f),
            "shadow camera y and z equal the oracle's (50.292, 1088.309)");
    require(near(shadow.world[12], centre[0] - 51.0f * light[8], 1e-3f), "shadow camera x is focus - 51 x forward (the oracle is 2.08 further, RENDER-74)");
    const auto clip = shadowViewProjection(shadow);
    const auto transform = [&](const std::array<float, 3>& p, int axis) {
        return clip[std::size_t(axis)] * p[0] + clip[std::size_t(4 + axis)] * p[1] + clip[std::size_t(8 + axis)] * p[2] + clip[std::size_t(12 + axis)];
    };
    require(near(transform(centre, 0), 0, 1e-4f) && near(transform(centre, 1), 0, 1e-4f), "the focus is the centre of the shadow texture");
    require(near(transform(centre, 2), 2.0f * 50.0f / 75.0f - 1.0f, 1e-4f), "the focus lies 51 along the depth range 1..76 (0x530850 scale 1/75)");
    const std::array<float, 3> edge = {centre[0] + 15.0f * light[0], centre[1] + 15.0f * light[1], centre[2] + 15.0f * light[2]};
    require(near(transform(edge, 0), 1.0f, 1e-4f), "x scale 1 / radius (0x530850 = 1/15)");
}

void lights() {
    std::vector<assets::LightObject> objects(3);
    objects[0].type = 0;
    objects[1].type = 2;
    objects[2].type = 1;
    require(!shadowLight(objects, std::nullopt), "no light casts shadows");
    objects[2].castShadows = true;
    objects[2].shadowOpacity = 1.4f;
    const auto light = shadowLight(objects, 20.0f);
    require(light && light->lightIndex == 2 && near(light->opacity, 1.0f, 1e-6f) && near(light->radius, 15.0f, 1e-6f),
            "the casting light, opacity clamped by 0x101864, the directional light's default radius 15 wins");
    objects[2].type = 4;
    require(near(shadowLight(objects, 20.0f)->radius, 20.0f, 1e-6f), "a specular casting light keeps the level's radius");
    objects[2].shadowOpacity.reset();
    require(near(shadowLight(objects, std::nullopt)->opacity, 0.5f, 1e-6f), "default ShadowOpacity 0.5 of 0x22dedc");
}

void flags() {
    assets::Level cls;
    assets::LevelParam caster;
    caster.hash = assets::nameHash("Shadow_Caster");
    caster.type = 5;
    caster.value = 1;
    assets::LevelParam receiver = caster;
    receiver.hash = assets::nameHash("Shadow_Receiver");
    receiver.value = 0;
    cls.defaults = {caster, receiver};
    const ShadowFlags character = shadowFlags(assets::LevelObject{}, cls);
    require(character.caster && !character.receiver, "Shadow_Caster 1, Shadow_Receiver 0");
    require(!shadowFlags(assets::LevelObject{}, assets::Level{}).caster, "a class without the parameters neither casts nor receives");
}

void realData(const std::string& fib, const std::string& names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const auto classOf = assets::archiveClassLookup(archive);
    const assets::FuseEntry* entry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    require(entry != nullptr, "missing lvl_01");
    const assets::Level level = assets::parseLevel(archive.read(*entry));
    const std::vector<assets::LightObject> objects = assets::lightObjects(level, classOf);
    const auto light = shadowLight(objects, assets::levelShadowRadius(level));
    require(light && objects[light->lightIndex].instance == "Directional01", "Directional01 casts the lvl_01 shadow (CastShadows 1)");
    require(near(light->opacity, 0.6f, 1e-6f) && light->colour == std::array<float, 3>{0, 0, 0} && near(light->radius, 15.0f, 1e-6f),
            "opacity 0.6 of class lightdirectional, black, radius 15 (oracle 0x377980, 0x56d400, 0x376800)");
    const auto expectFlags = [&](const char* className, bool casts, bool receives) {
        const assets::Level* cls = classOf(className);
        require(cls != nullptr, std::string("missing class ") + className);
        const ShadowFlags value = shadowFlags(assets::LevelObject{}, *cls);
        require(value.caster == casts && value.receiver == receives, std::string("shadow flags of ") + className);
    };
    expectFlags("chars/char_police", true, false);
    expectFlags("chars/char_chase_plainclothes", true, false);
    expectFlags("vehicles/vehicle_police_vigilant", true, false);
    expectFlags("props/parkour_railgrab", false, true);
}
}

int main(int argc, char** argv) {
    try {
        constant();
        focus();
        camera();
        lights();
        flags();
        if (argc >= 3) {
            if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) return 77;
            realData(argv[1], argv[2]);
        }
    } catch (const std::exception& error) {
        std::cerr << "shadow-test: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
