#pragma once

#include <array>
#include <optional>
#include <string>

namespace qol {

class Settings;

inline constexpr int viewDistanceFineSteps = 12;
inline constexpr int viewDistanceSteps = 28;

bool viewDistanceStepValid(int step);
float viewDistanceFactor(int step);
std::string viewDistanceText(int step);
std::string viewDistanceLabel(int step);
std::optional<int> parseViewDistance(const std::string& text);

int effectiveViewDistanceStep(const Settings& settings);
float effectiveViewDistanceFactor(const Settings& settings);

float viewDistance(float distance);
void scaleViewDistances(std::array<float, 3>& fadeIn, std::array<float, 3>& fadeOut);

template <class Table>
Table withViewDistance(Table table) {
    scaleViewDistances(table.fadeIn, table.fadeOut);
    return table;
}

template <class Planes>
Planes withFarPlane(Planes planes) {
    planes.farZ = viewDistance(planes.farZ);
    return planes;
}

template <class Fog>
Fog withFogDistance(Fog fog) {
    fog.start = viewDistance(fog.start);
    fog.end = viewDistance(fog.end);
    return fog;
}

}
