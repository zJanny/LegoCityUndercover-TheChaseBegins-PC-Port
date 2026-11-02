#include "qol/view_distance.hpp"

#include "qol/active_settings.hpp"

#include <stdexcept>

namespace qol {

namespace {

constexpr int quartersPerUnit = 4;
constexpr int firstQuarters = quartersPerUnit;
constexpr std::array<const char*, quartersPerUnit> quarterDigits{"0", "25", "5", "75"};

void requireStep(int step) {
    if (!viewDistanceStepValid(step))
        throw std::invalid_argument("view distance step " + std::to_string(step) + " is outside 0.." + std::to_string(viewDistanceSteps));
}

}

bool viewDistanceStepValid(int step) { return step >= 0 && step <= viewDistanceSteps; }

float viewDistanceFactor(int step) {
    requireStep(step);
    return static_cast<float>(firstQuarters + step) / static_cast<float>(quartersPerUnit);
}

std::string viewDistanceText(int step) {
    requireStep(step);
    const int quarters = firstQuarters + step;
    return std::to_string(quarters / quartersPerUnit) + "." + quarterDigits[static_cast<std::size_t>(quarters % quartersPerUnit)];
}

std::string viewDistanceLabel(int step) { return "x" + viewDistanceText(step); }

std::optional<int> parseViewDistance(const std::string& text) {
    for (int step = 0; step <= viewDistanceSteps; ++step)
        if (text == viewDistanceText(step)) return step;
    return std::nullopt;
}

int effectiveViewDistanceStep(const Settings& settings) {
    return settings.master() == Master::Original ? 0 : settings.viewDistanceStep();
}

float effectiveViewDistanceFactor(const Settings& settings) { return viewDistanceFactor(effectiveViewDistanceStep(settings)); }

float viewDistance(float distance) { return distance * effectiveViewDistanceFactor(activeSettings()); }

void scaleViewDistances(std::array<float, 3>& fadeIn, std::array<float, 3>& fadeOut) {
    const float factor = effectiveViewDistanceFactor(activeSettings());
    for (float& value : fadeIn) value *= factor;
    for (float& value : fadeOut) value *= factor;
}

}
