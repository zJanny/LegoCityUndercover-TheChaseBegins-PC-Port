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

void ScriptRunner::loadGeneratorNatives(const assets::ClassLookup& classOf) {
    propRocking_.load(level_, classOf);
    platforms_.load(level_, classOf);
}

void ScriptRunner::setPropRockingNative(const assets::ScriptCall& call) {
    if (call.arguments.size() < 2) return;
    propRocking_.setRocking(lowerName(call.arguments[0]), read(call.arguments[1]));
}

ScriptRunner::Result ScriptRunner::waitForPlatformNative(const assets::ScriptCall& call) {
    if (call.arguments.empty()) return Result::Continue;
    const PlatformRecord* record = platforms_.find(lowerName(call.arguments[0]));
    if (!record) return Result::Continue;
    return platformReached(*record) ? Result::Continue : Result::Block;
}

void ScriptRunner::platformMessage(const std::string& lowerInstance, std::uint8_t message) {
    if (message == kMessageTrigger) platforms_.triggerStandIn(lowerInstance);
}

}
