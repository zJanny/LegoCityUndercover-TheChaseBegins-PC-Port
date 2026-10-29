#pragma once

#include <cstddef>
#include <string>

namespace qol {

inline constexpr int maxVolume = 10;
inline constexpr int firstLanguage = 1;
inline constexpr int lastLanguage = 9;

class GameOptions {
public:
    virtual ~GameOptions() = default;
    virtual int sfxVolume() const = 0;
    virtual int musicVolume() const = 0;
    virtual bool invertCamera() const = 0;
    virtual int language() const = 0;
    virtual void setSfxVolume(int volume) = 0;
    virtual void setMusicVolume(int volume) = 0;
    virtual void setInvertCamera(bool invert) = 0;
    virtual void setLanguage(int language) = 0;
};

enum class GameRow { SfxVolume, MusicVolume, InvertCamera, Language };

inline constexpr std::size_t gameRowCount = 4;

const char* gameRowLabel(GameRow row);

enum class GameResult { Ignored, Changed, Rejected };

class GameTab {
public:
    explicit GameTab(GameOptions& options) : options_(options) {}

    int value(GameRow row) const;
    std::string valueText(GameRow row) const;

    GameResult step(GameRow row, int direction);
    GameResult activate(GameRow row);
    GameResult setValue(GameRow row, int value);

private:
    GameOptions& options_;
};

}
