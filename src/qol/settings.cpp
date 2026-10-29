#include "qol/settings.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace qol {

namespace {

[[noreturn]] void reject(std::size_t lineNumber, const std::string& text) {
    throw std::runtime_error("Invalid QoL configuration at line " +
        std::to_string(lineNumber) + ": " + text);
}

std::size_t indexOf(Feature feature) {
    return static_cast<std::size_t>(feature);
}

bool parseSide(const std::string& text, int& side) {
    if (text.empty() || text.size() > 5 || text[0] == '0') return false;
    int value = 0;
    for (char c : text) {
        if (c < '0' || c > '9') return false;
        value = value * 10 + (c - '0');
    }
    side = value;
    return true;
}

bool parseResolution(const std::string& text, Resolution& resolution) {
    const std::size_t split = text.find('x');
    if (split == std::string::npos) return false;
    Resolution parsed{};
    if (!parseSide(text.substr(0, split), parsed.width) ||
        !parseSide(text.substr(split + 1), parsed.height)) return false;
    if (!resolutionValid(parsed)) return false;
    resolution = parsed;
    return true;
}

}

const char* masterName(Master master) {
    switch (master) {
    case Master::Original: return "Original";
    case Master::Improved: return "Improved";
    case Master::Custom: return "Custom";
    }
    return "Original";
}

const char* windowModeName(WindowMode mode) {
    return mode == WindowMode::Fullscreen ? "fullscreen" : "windowed";
}

bool resolutionValid(const Resolution& resolution) {
    return resolution.width >= minimumResolution.width &&
        resolution.height >= minimumResolution.height;
}

std::string resolutionText(const Resolution& resolution) {
    return std::to_string(resolution.width) + "x" + std::to_string(resolution.height);
}

bool Settings::setResolution(const Resolution& resolution) {
    if (!resolutionValid(resolution)) return false;
    resolution_ = resolution;
    return true;
}

std::optional<Resolution> Settings::effectiveResolution(const Resolution& desktop) const {
    if (resolution_) return resolution_;
    if (!resolutionValid(desktop)) return std::nullopt;
    return desktop;
}

bool Settings::customSwitch(Feature feature) const {
    return switches_.at(indexOf(feature));
}

bool Settings::enabled(Feature feature) const {
    switch (master_) {
    case Master::Original: return false;
    case Master::Improved: return true;
    case Master::Custom: return customSwitch(feature);
    }
    return false;
}

bool Settings::setMaster(Master master) {
    if (master == Master::Custom && !mixSet_) return false;
    master_ = master;
    return true;
}

Master Settings::cycleMaster() {
    switch (master_) {
    case Master::Original:
        master_ = Master::Improved;
        break;
    case Master::Improved:
        master_ = mixSet_ ? Master::Custom : Master::Original;
        break;
    case Master::Custom:
        master_ = Master::Original;
        break;
    }
    return master_;
}

void Settings::setSwitch(Feature feature, bool on) {
    switches_.at(indexOf(feature)) = on;
    mixSet_ = true;
}

Settings readSettings(std::istream& stream) {
    Settings settings;
    std::array<bool, featureCount> seen{};
    bool masterSeen = false;
    bool windowModeSeen = false;
    bool resolutionSeen = false;
    bool wantCustom = false;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(stream, line)) {
        ++lineNumber;
        std::istringstream row(line.substr(0, line.find('#')));
        std::string name, value, extra;
        if (!(row >> name)) continue;
        if (!(row >> value) || (row >> extra)) reject(lineNumber, line);
        if (name == "master") {
            if (masterSeen) reject(lineNumber, line);
            masterSeen = true;
            if (value == "Original") settings.setMaster(Master::Original);
            else if (value == "Improved") settings.setMaster(Master::Improved);
            else if (value == "Custom") wantCustom = true;
            else reject(lineNumber, line);
            continue;
        }
        if (name == "window_mode") {
            if (windowModeSeen) reject(lineNumber, line);
            windowModeSeen = true;
            if (value == "windowed") settings.setWindowMode(WindowMode::Windowed);
            else if (value == "fullscreen") settings.setWindowMode(WindowMode::Fullscreen);
            else reject(lineNumber, line);
            continue;
        }
        if (name == "resolution") {
            if (resolutionSeen) reject(lineNumber, line);
            resolutionSeen = true;
            Resolution parsed{};
            if (!parseResolution(value, parsed)) reject(lineNumber, line);
            settings.setResolution(parsed);
            continue;
        }
        bool found = false;
        for (const FeatureInfo& info : featureTable) {
            if (name != info.name) continue;
            found = true;
            if (seen[indexOf(info.feature)]) reject(lineNumber, line);
            seen[indexOf(info.feature)] = true;
            if (value != "on" && value != "off") reject(lineNumber, line);
            settings.setSwitch(info.feature, value == "on");
        }
        if (!found) reject(lineNumber, line);
    }
    if (wantCustom && !settings.setMaster(Master::Custom))
        throw std::runtime_error("Invalid QoL configuration: master Custom without a stored mix");
    return settings;
}

void writeSettings(std::ostream& stream, const Settings& settings) {
    stream << "master " << masterName(settings.master()) << '\n';
    stream << "window_mode " << windowModeName(settings.windowMode()) << '\n';
    if (settings.resolution())
        stream << "resolution " << resolutionText(*settings.resolution()) << '\n';
    if (!settings.mixSet()) return;
    for (const FeatureInfo& info : featureTable)
        stream << info.name << ' ' << (settings.customSwitch(info.feature) ? "on" : "off") << '\n';
}

Settings loadSettings(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        if (!std::filesystem::exists(path)) return Settings{};
        throw std::runtime_error("Cannot read QoL configuration " + path);
    }
    return readSettings(file);
}

void saveSettings(const std::string& path, const Settings& settings) {
    const std::string temporary = path + ".tmp";
    {
        std::ofstream file(temporary, std::ios::trunc);
        if (!file) throw std::runtime_error("Cannot write QoL configuration " + path);
        writeSettings(file, settings);
        file.flush();
        if (!file) throw std::runtime_error("Cannot write QoL configuration " + path);
    }
    std::filesystem::rename(temporary, path);
}

}
