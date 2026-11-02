#include "runtime/script_runner.hpp"

#include <algorithm>
#include <cctype>

namespace chase::runtime {
namespace {

std::string lowerName(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

constexpr unsigned kBulletTimeArguments = 3;

}

void ScriptRunner::bulletTimeNative(const assets::ScriptCall& call) {
    if (call.arguments.size() < kBulletTimeArguments) return;
    bulletTime_.set(read(call.arguments[0]), read(call.arguments[1]));
}

void ScriptRunner::vehicleAiNative(const assets::ScriptCall& call, bool chased) {
    const auto& args = call.arguments;
    if (args.size() < (chased ? kVehicleSetChasedArguments : kVehicleSetAiArguments)) return;
    const std::string vehicle = lowerName(args[0]);
    float where[3] = {0, 0, 0};
    if (objectPosition_) objectPosition_(vehicle, where);
    AiMatrix node{};
    node[0] = {1.0f, 0.0f, 0.0f};
    node[1] = {0.0f, 1.0f, 0.0f};
    node[2] = {0.0f, 0.0f, 1.0f};
    node[3] = {where[0], where[1], where[2]};
    if (chased) vehicleAi_.setChased(vehicle, node, assets::nameHash(args[1]), read(args[2]));
    else vehicleAi_.setAi(vehicle, node);
}

void ScriptRunner::spawnDebris2ColourNative(const assets::ScriptCall& call) {
    const auto& args = call.arguments;
    if (args.size() < kSpawnDebris2ColourArguments) return;
    const std::string object = lowerName(args[0]);
    float where[3];
    if (!objectPosition_ || !objectPosition_(object, where)) return;
    PropDestroyEffects classEffects;
    if (classOf_)
        for (const assets::LevelObject& placed : level_.objects)
            if (lowerName(placed.instance) == object) {
                if (const assets::Level* cls = classOf_(placed.className)) classEffects = propDestroyEffects(placed, *cls);
                break;
            }
    const std::uint32_t colour = scriptDebrisColour(read(args[1]), read(args[2]), read(args[3]));
    const std::uint32_t second = scriptDebrisColour(read(args[4]), read(args[5]), read(args[6]));
    debrisRequests_.push_back({object, scriptDebrisBurst(classEffects, {where[0], where[1], where[2]}, colour, second)});
}

std::vector<ScriptDebrisRequest> ScriptRunner::takeDebrisRequests() {
    std::vector<ScriptDebrisRequest> taken;
    taken.swap(debrisRequests_);
    return taken;
}

}
