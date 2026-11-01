// The TV crew of mission 1-3 in the level data of lvl_01_cherrytreehills (docs/re/script-natives-animation.md, entry "Bone attaches of characters"):
// the four AttachToBoneRot calls of the level's start script, the camera and boom objects they name, the crew's skeleton and the local matrix the
// attach gives each prop. Needs the extracted RomFS; exits with 77 (skipped) when it is missing. Values printed with the C++ level and skeleton
// loaders by 20261101-mission-A on 2026-11-01.
#include "assets/assets.hpp"
#include "runtime/bone_attach.hpp"
#include "runtime/natives_animation.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase;
constexpr const char* kLevelFile = "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl";

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

struct Expected {
    const char* holder;
    const char* prop;
    const char* propMesh;
    float degrees;
};

constexpr Expected kCrew[] = {
    {"1-3_Camera_Man_chopper", "1-3_Camera_chopper", "Weap_VideoCamera", 45.0f},
    {"1-3_Sound_Man_chopper", "1-3_Boom_chopper", "Weap_SoundBoom/Weap_SoundBoom_atlas_lvl_01_cherrytreehills_main", 60.0f},
    {"1-3_Camera_Man_park", "1-3_Camera_park", "Weap_VideoCamera", 45.0f},
    {"1-3_Sound_Man_park", "1-3_Boom_park", "Weap_SoundBoom/Weap_SoundBoom_atlas_lvl_01_cherrytreehills_main", 60.0f},
};

bool near(float a, float b) { return std::fabs(a - b) < 1e-5f; }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: bone-attach-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "bone-attach-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::Level level = assets::parseLevel(archive.read(*archive.find(kLevelFile)));
        std::vector<assets::ScriptCall> calls;
        for (const assets::LevelScript& script : level.scripts)
            for (const assets::ScriptCall& call : script.calls)
                if (lower(call.function) == "attachtobonerot") calls.push_back(call);
        require(calls.size() == 4, "the level has four AttachToBoneRot calls");

        const assets::DrawableObjects drawable = assets::drawableObjects(archive, level);
        for (const Expected& crew : kCrew) {
            const auto call = std::find_if(calls.begin(), calls.end(), [&](const assets::ScriptCall& c) { return lower(c.arguments[0]) == lower(crew.holder); });
            require(call != calls.end(), std::string("AttachToBoneRot on ") + crew.holder);
            require(call->arguments.size() == 6 && lower(call->arguments[1]) == lower(crew.prop) && lower(call->arguments[2]) == "\"weaponright\"", "arguments of the call");
            require(std::stof(call->arguments[3]) == crew.degrees && std::stof(call->arguments[4]) == 0.0f && std::stof(call->arguments[5]) == 0.0f, "the angles");

            const auto prop = std::find_if(drawable.objects.begin(), drawable.objects.end(), [&](const assets::Placement& p) { return lower(p.instance) == lower(crew.prop); });
            require(prop != drawable.objects.end() && prop->mesh == crew.propMesh, std::string("placed prop with a model: ") + crew.prop);
            require(archive.find(assets::meshModelPath(prop->mesh)) != nullptr, std::string("the model of ") + crew.prop + " is in the archive");

            const auto holder = std::find_if(drawable.characters.begin(), drawable.characters.end(), [&](const assets::CharacterObject& c) { return lower(c.placement.instance) == lower(crew.holder); });
            require(holder != drawable.characters.end() && holder->appearance.body == "char_minifig", std::string("holder is a minifig character: ") + crew.holder);
            const std::string bodyPath = assets::meshModelPath(holder->appearance.body);
            const assets::Skeleton skeleton = assets::parseSkeleton(archive.read(*archive.find(bodyPath.substr(0, bodyPath.rfind('.')) + ".bfnskl")));
            const auto joint = std::find_if(skeleton.joints.begin(), skeleton.joints.end(), [](const assets::Joint& j) { return lower(j.name) == "weaponright"; });
            require(joint != skeleton.joints.end(), "the skeleton has the joint weaponRight");
            require(near(joint->world[12], 0.484492183f) && near(joint->world[13], 0.764553666f) && near(joint->world[14], 0.305616587f),
                    "bind translation of weaponRight");

            runtime::Attachment attachment;
            attachment.kind = runtime::AttachKind::BoneRotated;
            attachment.parent = lower(crew.holder);
            attachment.parentBone = "weaponright";
            attachment.sequence = 1;
            for (int axis = 0; axis < 3; ++axis) attachment.rotationDegrees[axis] = std::stof(call->arguments[std::size_t(3 + axis)]);
            runtime::BoneAttachments links;
            require(links.attach(lower(crew.prop), attachment, joint->world), "taken");
            const assets::Matrix& local = links.find(lower(crew.prop))->local;
            const float radians = crew.degrees * 0.0174532942f;
            require(near(local[0], 1.0f) && near(local[5], std::cos(radians)) && near(local[6], std::sin(radians)) && near(local[9], -std::sin(radians)) && near(local[10], std::cos(radians)),
                    "the local rotation is the angle about x");
            require(near(local[12], joint->world[12]) && near(local[13], joint->world[13]) && near(local[14], joint->world[14]), "the local translation is the bind translation");

            const assets::Matrix identity = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
            const assets::Matrix standing = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 100, 20, 300, 1};
            const assets::Matrix world = runtime::boneAttachedWorld(local, identity, standing);
            require(near(world[12], 100 + joint->world[12]) && near(world[13], 20 + joint->world[13]) && near(world[14], 300 + joint->world[14]),
                    "in the bind pose the prop sits at the joint's bind position");
        }
    } catch (const std::exception& error) {
        std::cerr << "bone-attach-real-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "bone-attach-real-test: ok\n";
    return 0;
}
