#pragma once

#include "qol/features.hpp"

#include <array>
#include <iosfwd>
#include <optional>
#include <string>

namespace qol {

enum class Master { Original, Improved, Custom };

enum class WindowMode { Windowed, Fullscreen };

struct Resolution {
    int width;
    int height;
    bool operator==(const Resolution& other) const {
        return width == other.width && height == other.height;
    }
};

inline constexpr Resolution minimumResolution{400, 240};
inline constexpr WindowMode defaultWindowMode = WindowMode::Fullscreen;

const char* masterName(Master master);
const char* windowModeName(WindowMode mode);
bool resolutionValid(const Resolution& resolution);
std::string resolutionText(const Resolution& resolution);

class Settings {
public:
    Master master() const { return master_; }
    bool mixSet() const { return mixSet_; }
    bool customSwitch(Feature feature) const;
    bool enabled(Feature feature) const;
    WindowMode windowMode() const { return windowMode_; }
    const std::optional<Resolution>& resolution() const { return resolution_; }
    std::optional<Resolution> effectiveResolution(const Resolution& desktop) const;

    bool setMaster(Master master);
    Master cycleMaster();
    void setSwitch(Feature feature, bool on);
    void setWindowMode(WindowMode mode) { windowMode_ = mode; }
    bool setResolution(const Resolution& resolution);

private:
    Master master_ = Master::Original;
    bool mixSet_ = false;
    std::array<bool, featureCount> switches_{};
    WindowMode windowMode_ = defaultWindowMode;
    std::optional<Resolution> resolution_;
};

Settings readSettings(std::istream& stream);
void writeSettings(std::ostream& stream, const Settings& settings);
Settings loadSettings(const std::string& path);
void saveSettings(const std::string& path, const Settings& settings);

}
