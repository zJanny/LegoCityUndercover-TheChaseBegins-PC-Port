#include "runtime/platform_riding.hpp"
#include "runtime/script_runner.hpp"

#include <algorithm>
#include <cctype>

namespace chase::runtime {
namespace {

std::string lowerName(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

}

void ScriptRunner::platformMessage(const std::string& lowerInstance, std::uint8_t message) { platforms_.message(lowerInstance, message); }

void ScriptRunner::platformNative(const assets::ScriptCall& call) {
    const std::string name = lowerName(call.function);
    const auto& args = call.arguments;
    if (args.size() < 2) return;
    PlatformRecord* record = platforms_.find(lowerName(args[0]));
    if (!record) return;
    if (name == "setplatformspeed") {
        setPlatformSpeed(*record, read(args[1]));
    } else if (name == "moveplatform") {
        if (args.size() >= 3) movePlatform(*record, read(args[1]), read(args[2]) != 0.0f);
    } else {
        float position[3] = {0, 0, 0};
        if (!objectPosition_ || !objectPosition_(lowerName(args[1]), position)) return;
        const PlatformVector translation{position[0], position[1], position[2]};
        if (name == "setplatformorigin") setPlatformOrigin(*record, translation);
        else setPlatformTarget(*record, translation);
    }
}

void ScriptRunner::platformAttachment(AnimationNativeKind kind, const std::vector<std::string>& args) {
    const bool rootOrRoom = kind == AnimationNativeKind::AttachToWorldRoot || kind == AnimationNativeKind::AttachToRoom;
    const bool underParent = kind == AnimationNativeKind::AttachObject || kind == AnimationNativeKind::AttachToBone || kind == AnimationNativeKind::AttachToBoneRot;
    if ((!rootOrRoom && !underParent) || args.size() < animationNativeArguments(kind)) return;
    const std::string child = lowerName(rootOrRoom ? args[0] : args[1]);
    if (rootOrRoom) {
        platforms_.setNodeParent(child, "");
        return;
    }
    const std::string parent = lowerName(args[0]);
    if (kind == AnimationNativeKind::AttachObject) platforms_.attachObject(parent, child);
    else platforms_.setNodeParent(child, parent);
}

}
