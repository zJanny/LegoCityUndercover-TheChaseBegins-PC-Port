#include "qol/mouse_sensitivity.hpp"

#include "qol/active_settings.hpp"

#include <array>
#include <stdexcept>

namespace qol {

namespace {

constexpr int quartersPerUnit = 4;
constexpr int firstQuarters = 1;
constexpr std::array<const char*, quartersPerUnit> quarterDigits{"0", "25", "5", "75"};

void requireStep(int step) {
    if (!mouseSensitivityStepValid(step))
        throw std::invalid_argument("mouse sensitivity step " + std::to_string(step) + " is outside 0.." + std::to_string(mouseSensitivitySteps));
}

}

bool mouseSensitivityStepValid(int step) { return step >= 0 && step <= mouseSensitivitySteps; }

float mouseSensitivityFactor(int step) {
    requireStep(step);
    return static_cast<float>(firstQuarters + step) / static_cast<float>(quartersPerUnit);
}

std::string mouseSensitivityText(int step) {
    requireStep(step);
    const int quarters = firstQuarters + step;
    return std::to_string(quarters / quartersPerUnit) + "." + quarterDigits[static_cast<std::size_t>(quarters % quartersPerUnit)];
}

std::string mouseSensitivityLabel(int step) { return "x" + mouseSensitivityText(step); }

std::optional<int> parseMouseSensitivity(const std::string& text) {
    for (int step = 0; step <= mouseSensitivitySteps; ++step)
        if (text == mouseSensitivityText(step)) return step;
    return std::nullopt;
}

float mouseSecondsPerPixel(const Settings& settings) {
    return baseMouseShoulderSecondsPerPixel * mouseSensitivityFactor(settings.mouseSensitivityStep());
}

float mouseSecondsPerPixel() { return mouseSecondsPerPixel(activeSettings()); }

}
