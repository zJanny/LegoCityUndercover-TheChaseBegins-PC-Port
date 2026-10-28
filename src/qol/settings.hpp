#pragma once

#include "qol/features.hpp"

#include <array>
#include <iosfwd>
#include <string>

namespace qol {

enum class Master { Original, Improved, Custom };

const char* masterName(Master master);

class Settings {
public:
    Master master() const { return master_; }
    bool mixSet() const { return mixSet_; }
    bool customSwitch(Feature feature) const;
    bool enabled(Feature feature) const;

    bool setMaster(Master master);
    Master cycleMaster();
    void setSwitch(Feature feature, bool on);

private:
    Master master_ = Master::Original;
    bool mixSet_ = false;
    std::array<bool, featureCount> switches_{};
};

Settings readSettings(std::istream& stream);
void writeSettings(std::ostream& stream, const Settings& settings);
Settings loadSettings(const std::string& path);
void saveSettings(const std::string& path, const Settings& settings);

}
