#pragma once

#include <optional>
#include <string>

namespace qol {

class Settings;

inline constexpr int mouseSensitivitySteps = 11;
inline constexpr int defaultMouseSensitivityStep = 3;
inline constexpr float baseMouseShoulderSecondsPerPixel = 0.0018f;

bool mouseSensitivityStepValid(int step);
float mouseSensitivityFactor(int step);
std::string mouseSensitivityText(int step);
std::string mouseSensitivityLabel(int step);
std::optional<int> parseMouseSensitivity(const std::string& text);

float mouseSecondsPerPixel(const Settings& settings);
float mouseSecondsPerPixel();

}
