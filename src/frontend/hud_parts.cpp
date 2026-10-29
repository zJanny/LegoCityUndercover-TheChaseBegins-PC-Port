#include "frontend/hud_parts.hpp"

namespace chase::frontend {

const std::set<std::string>& hudPartLayoutElements(HudPart part) {
    static const std::set<std::string> portrait = {"UI_HUD_3DS"};
    static const std::set<std::string> hearts = {"UI_Hearts"};
    static const std::set<std::string> studs = {"Stud_Count", "CombatStud_Count", "UI_Stud"};
    static const std::set<std::string> none;
    switch (part) {
    case HudPart::Portrait: return portrait;
    case HudPart::Hearts: return hearts;
    case HudPart::Studs: return studs;
    case HudPart::SuperBricks:
    case HudPart::CameraLock: return none;
    }
    return none;
}

std::set<std::string> hudHiddenLayoutElements(HudPartSet parts) {
    std::set<std::string> hidden;
    for (int i = 0; i < kHudPartCount; ++i) {
        const HudPart part = HudPart(i);
        if (parts.contains(part)) continue;
        const auto& names = hudPartLayoutElements(part);
        hidden.insert(names.begin(), names.end());
    }
    return hidden;
}

}
