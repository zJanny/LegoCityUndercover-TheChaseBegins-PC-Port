#include "runtime/target_hearts_display.hpp"

namespace chase::runtime {

namespace {

std::string portraitPath(const std::string& name) {
    const std::string format = kTargetPortraitFormat;
    const std::size_t at = format.find("%s");
    return format.substr(0, at) + name + format.substr(at + 2);
}

}

void setTargetHearts(TargetHeartsDisplay& display, const TargetHeartsSource& source) {
    std::uint16_t most = 0;
    std::uint16_t hearts = 0;
    if (source.kind == TargetHeartsKind::Character || source.kind == TargetHeartsKind::Vehicle) {
        most = source.most;
        hearts = source.hearts;
        display.portrait = portraitPath(source.portraitName);
    }
    if (display.most != most) {
        display.previous = most;
        display.hearts = most;
        display.most = most;
    }
    display.shown = most != 0 && most != kTargetHeartsNoLimit;
    display.previous = display.hearts;
    const bool changed = display.hearts != hearts;
    display.hearts = hearts;
    if (changed || display.most == kTargetHeartsNoLimit) {
        display.changed = 1.0f;
        display.unchanged = 0.0f;
    } else {
        display.changed = 0.0f;
        display.unchanged = 1.0f;
    }
    display.target = source.kind == TargetHeartsKind::None ? std::string() : source.entity;
}

}
