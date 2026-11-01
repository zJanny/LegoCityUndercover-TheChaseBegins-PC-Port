#pragma once

#include "qol/settings.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace qol {

struct DisplayValues {
    WindowMode mode;
    std::optional<Resolution> resolution;
    bool operator==(const DisplayValues& other) const {
        return mode == other.mode && resolution == other.resolution;
    }
    bool operator!=(const DisplayValues& other) const { return !(*this == other); }
};

DisplayValues displayValuesOf(const Settings& settings);

using WindowApplier = std::function<void(const Resolution&, WindowMode)>;

enum class KeepButton { Keep, Revert };
enum class KeepOutcome { None, Kept, Reverted };

struct KeepDialog {
    std::string title;
    std::string body;
    std::vector<std::string> buttons;
    KeepButton defaultFocus;
};

inline constexpr double keepSettingsSeconds = 15.0;

class KeepSettings {
public:
    KeepSettings(Settings& stored, Resolution desktop, WindowApplier applier);

    void begin(const DisplayValues& previous, const DisplayValues& current, double nowSeconds);
    KeepOutcome tick(double nowSeconds);
    KeepOutcome confirm();
    KeepOutcome cancel();
    KeepOutcome choose(KeepButton button);

    bool active() const { return active_; }
    int secondsLeft() const;
    std::string text() const;
    std::optional<KeepDialog> dialog() const;
    const DisplayValues& previous() const { return previous_; }

private:
    KeepOutcome revert();

    Settings& stored_;
    Resolution desktop_;
    WindowApplier applier_;
    bool active_ = false;
    DisplayValues previous_{defaultWindowMode, std::nullopt};
    double deadline_ = 0;
    double lastNow_ = 0;
};

}
