// Spec: docs/design/qol-settings.md entries 2026-10-28T09:32:07 (default off) and 09:53:33 (Original all off, Improved all on,
// per-feature switches only in Custom). Cases written by 20261029-qolview-A on 2026-10-29.
#include "qol/active_settings.hpp"

#include <iostream>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

}

int main() {
    require(qol::activeSettings().master() == qol::Master::Original, "default master is Original");
    require(!qol::enabled(qol::Feature::WidenedView), "default widened view off");

    qol::Settings improved;
    improved.setMaster(qol::Master::Improved);
    qol::setActiveSettings(improved);
    require(qol::enabled(qol::Feature::WidenedView), "Improved turns the widened view on");

    qol::Settings custom;
    custom.setSwitch(qol::Feature::WidenedView, true);
    custom.setMaster(qol::Master::Custom);
    qol::setActiveSettings(custom);
    require(qol::activeSettings().master() == qol::Master::Custom, "store keeps Custom");
    require(qol::enabled(qol::Feature::WidenedView), "Custom with the switch on");

    custom.setSwitch(qol::Feature::WidenedView, false);
    qol::setActiveSettings(custom);
    require(!qol::enabled(qol::Feature::WidenedView), "Custom with the switch off");

    qol::setActiveSettings(qol::Settings{});
    require(!qol::enabled(qol::Feature::WidenedView), "back to Original");

    if (failures) return 1;
    std::cout << "qol active settings: ok\n";
    return 0;
}
