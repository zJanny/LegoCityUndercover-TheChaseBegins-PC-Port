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
